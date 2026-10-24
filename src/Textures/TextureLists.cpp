#include "Textures/TextureLists.h"

#include "Color/PubesColor.h"

namespace {
    constexpr auto FemaleDirectory{"Data/meshes/opubes"};
    constexpr auto MaleDirectory{"Data/meshes/opubes/male"};

    void ReadFile(const std::filesystem::path& a_file, TextureLists::Lists& a_lists) {
        // From the pubes folder, so the female and male files with the same name can be told apart
        const auto fileName{a_file.lexically_relative(FemaleDirectory).generic_string()};

        std::ifstream stream{a_file, std::ios::binary};
        if (!stream) {
            logger::warn("Couldn't open {}", a_file.string());
            return;
        }

        // Like OBody, so files saved with a BOM or as UTF-16 are read too
        rapidjson::IStreamWrapper wrapper{stream};
        rapidjson::AutoUTFInputStream<unsigned, rapidjson::IStreamWrapper> encodedStream{wrapper};
        rapidjson::Document document;
        if (document.ParseStream<0, rapidjson::AutoUTF<unsigned>>(encodedStream).HasParseError()) {
            logger::warn("Couldn't parse {} (offset {}): {}", fileName, document.GetErrorOffset(),
                         rapidjson::GetParseError_En(document.GetParseError()));
            return;
        }

        if (!document.IsObject()) {
            logger::warn("{} isn't a JSON object of textures and their types", fileName);
            return;
        }

        std::size_t count{0};
        // Not GetObject, windows.h has a GetObject macro that turns it into GetObjectA
        for (auto member{document.MemberBegin()}; member != document.MemberEnd(); ++member) {
            const auto& value{member->value};
            const std::string texture{member->name.GetString(), member->name.GetStringLength()};

            if (std::error_code error; !std::filesystem::exists(texture, error)) {
                logger::info("{} isn't installed, skipping the rest of {}", texture, fileName);
                break;
            }

            const std::string_view type{value.IsString() ? value.GetString() : ""};
            if (type == "normal") {
                a_lists.normal.push_back(texture);
            } else if (type == "stylish") {
                a_lists.stylish.push_back(texture);
            } else if (type == "hairy") {
                a_lists.hairy.push_back(texture);
            } else {
                logger::warn("Unknown type \"{}\" for {} in {}", type, texture, fileName);
                continue;
            }
            ++count;
        }

        logger::info("{} pubes textures in {}", count, fileName);
    }

    TextureLists::Lists Read(const std::filesystem::path& a_directory) {
        TextureLists::Lists lists;

        // Not having any male pubes installed is normal
        if (std::error_code error; !std::filesystem::is_directory(a_directory, error)) {
            logger::info("No {} folder", a_directory.string());
            return lists;
        }

        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator{a_directory, error}) {
            if (std::error_code fileError; entry.is_regular_file(fileError) && entry.path().extension() == ".json") {
                ReadFile(entry.path(), lists);
            }
        }

        if (error) {
            logger::warn("Couldn't read the JSON files in {}: {}", a_directory.string(), error.message());
        }

        return lists;
    }

    // Only written by Load, before anything reads them
    TextureLists::Lists g_female;
    TextureLists::Lists g_male;
    std::unordered_set<std::string> g_allTextures;

    void AddList(const std::vector<std::string>& a_textures) {
        PubesColor::CacheAverageColors(a_textures);

        for (const auto& texture : a_textures) {
            g_allTextures.insert(PubesColor::NormalizePath(texture));
        }
    }

    void AddLists(const TextureLists::Lists& a_lists) {
        AddList(a_lists.normal);
        AddList(a_lists.stylish);
        AddList(a_lists.hairy);
    }
}  // namespace

void TextureLists::Load() {
    g_female = Read(FemaleDirectory);
    g_male = Read(MaleDirectory);

    // Males too, whether they get pubes is set in the MCM, which isn't known until a save is loaded
    AddLists(g_female);
    AddLists(g_male);
}

const TextureLists::Lists& TextureLists::GetFemale() { return g_female; }

const TextureLists::Lists& TextureLists::GetMale() { return g_male; }

bool TextureLists::IsPubesTexture(const std::string_view a_texture) {
    return g_allTextures.contains(PubesColor::NormalizePath(a_texture));
}
