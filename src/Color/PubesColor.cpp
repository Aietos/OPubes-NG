#include "Color/PubesColor.h"

#define BCDEC_IMPLEMENTATION
#include <bcdec.h>

namespace {
    // DDS file layout, see https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
    constexpr std::uint32_t DDSMagic{0x20534444};  // "DDS "

    constexpr std::uint32_t MakeFourCC(const char (&a_code)[5]) {
        return static_cast<std::uint32_t>(a_code[0]) | static_cast<std::uint32_t>(a_code[1]) << 8 |
               static_cast<std::uint32_t>(a_code[2]) << 16 | static_cast<std::uint32_t>(a_code[3]) << 24;
    }

    constexpr std::uint32_t DDPFAlphaPixels{0x1};
    constexpr std::uint32_t DDPFFourCC{0x4};
    constexpr std::uint32_t DDPFRGB{0x40};

    struct DDSPixelFormat {
        std::uint32_t size;
        std::uint32_t flags;
        std::uint32_t fourCC;
        std::uint32_t rgbBitCount;
        std::uint32_t rBitMask;
        std::uint32_t gBitMask;
        std::uint32_t bBitMask;
        std::uint32_t aBitMask;
    };

    struct DDSHeader {
        std::uint32_t size;
        std::uint32_t flags;
        std::uint32_t height;
        std::uint32_t width;
        std::uint32_t pitchOrLinearSize;
        std::uint32_t depth;
        std::uint32_t mipMapCount;
        std::uint32_t reserved1[11];
        DDSPixelFormat pixelFormat;
        std::uint32_t caps;
        std::uint32_t caps2;
        std::uint32_t caps3;
        std::uint32_t caps4;
        std::uint32_t reserved2;
    };
    static_assert(sizeof(DDSHeader) == 124);

    struct DDSHeaderDX10 {
        std::uint32_t dxgiFormat;
        std::uint32_t resourceDimension;
        std::uint32_t miscFlag;
        std::uint32_t arraySize;
        std::uint32_t miscFlags2;
    };
    static_assert(sizeof(DDSHeaderDX10) == 20);

    enum class Format { BC1, BC2, BC3, BC7, RGBA8, BGRA8, BGRX8 };

    std::string_view FormatName(const Format a_format) {
        switch (a_format) {
            case Format::BC1:
                return "BC1";
            case Format::BC2:
                return "BC2";
            case Format::BC3:
                return "BC3";
            case Format::BC7:
                return "BC7";
            case Format::RGBA8:
                return "RGBA8";
            case Format::BGRA8:
                return "BGRA8";
            default:
                return "BGRX8";
        }
    }

    bool IsBlockCompressed(const Format a_format) {
        return a_format == Format::BC1 || a_format == Format::BC2 || a_format == Format::BC3 || a_format == Format::BC7;
    }

    // BC1 blocks are 8 bytes, BC2, BC3 and BC7 ones are 16
    std::size_t GetBlockSize(const Format a_format) {
        return static_cast<std::size_t>(a_format == Format::BC1 ? BCDEC_BC1_BLOCK_SIZE : BCDEC_BC7_BLOCK_SIZE);
    }

    std::size_t GetLevelSize(const Format a_format, const std::uint32_t a_width, const std::uint32_t a_height) {
        if (IsBlockCompressed(a_format)) {
            return static_cast<std::size_t>(std::max(1u, (a_width + 3) / 4)) * std::max(1u, (a_height + 3) / 4) *
                   GetBlockSize(a_format);
        }
        return static_cast<std::size_t>(a_width) * a_height * 4;
    }

    std::optional<Format> GetDX10Format(const std::uint32_t a_dxgiFormat) {
        switch (a_dxgiFormat) {
            case 70:  // DXGI_FORMAT_BC1_TYPELESS
            case 71:  // DXGI_FORMAT_BC1_UNORM
            case 72:  // DXGI_FORMAT_BC1_UNORM_SRGB
                return Format::BC1;
            case 73:  // DXGI_FORMAT_BC2_TYPELESS
            case 74:  // DXGI_FORMAT_BC2_UNORM
            case 75:  // DXGI_FORMAT_BC2_UNORM_SRGB
                return Format::BC2;
            case 76:  // DXGI_FORMAT_BC3_TYPELESS
            case 77:  // DXGI_FORMAT_BC3_UNORM
            case 78:  // DXGI_FORMAT_BC3_UNORM_SRGB
                return Format::BC3;
            case 97:  // DXGI_FORMAT_BC7_TYPELESS
            case 98:  // DXGI_FORMAT_BC7_UNORM
            case 99:  // DXGI_FORMAT_BC7_UNORM_SRGB
                return Format::BC7;
            case 27:  // DXGI_FORMAT_R8G8B8A8_TYPELESS
            case 28:  // DXGI_FORMAT_R8G8B8A8_UNORM
            case 29:  // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
                return Format::RGBA8;
            case 87:  // DXGI_FORMAT_B8G8R8A8_UNORM
            case 90:  // DXGI_FORMAT_B8G8R8A8_TYPELESS
            case 91:  // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
                return Format::BGRA8;
            default:
                return std::nullopt;
        }
    }

    std::optional<Format> GetLegacyFormat(const DDSPixelFormat& a_pixelFormat) {
        if (a_pixelFormat.flags & DDPFFourCC) {
            switch (a_pixelFormat.fourCC) {
                case MakeFourCC("DXT1"):
                    return Format::BC1;
                case MakeFourCC("DXT2"):
                case MakeFourCC("DXT3"):
                    return Format::BC2;
                case MakeFourCC("DXT4"):
                case MakeFourCC("DXT5"):
                    return Format::BC3;
                default:
                    return std::nullopt;
            }
        }

        if ((a_pixelFormat.flags & DDPFRGB) && a_pixelFormat.rgbBitCount == 32) {
            const bool hasAlpha{(a_pixelFormat.flags & DDPFAlphaPixels) && a_pixelFormat.aBitMask != 0};
            if (a_pixelFormat.rBitMask == 0x000000FF) return Format::RGBA8;
            if (a_pixelFormat.rBitMask == 0x00FF0000) return hasAlpha ? Format::BGRA8 : Format::BGRX8;
        }

        return std::nullopt;
    }

    // NiBinaryStream::read only tells if it read all the bytes asked for, which is fine since the DDS header says how
    // many bytes there are up to the mip level that's read
    template <class T>
    bool ReadInto(RE::BSResourceNiBinaryStream& a_stream, T& a_value) {
        return a_stream.read(reinterpret_cast<std::byte*>(&a_value), static_cast<std::uint32_t>(sizeof(T)));
    }

    // Every pixel weighted by its alpha, which is how much of it shows in game. Most of what shows of pubes is half
    // transparent, and those parts are lighter than the opaque centers of the strands
    struct ColorSums {
        std::array<double, 3> weighted{};
        double weight{0.0};

        void Add(const std::uint8_t a_red, const std::uint8_t a_green, const std::uint8_t a_blue,
                 const std::uint8_t a_alpha) {
            const double alpha{a_alpha / 255.0};
            weighted[0] += a_red * alpha;
            weighted[1] += a_green * alpha;
            weighted[2] += a_blue * alpha;
            weight += alpha;
        }

        void Add(const ColorSums& a_other) {
            for (std::size_t channel = 0; channel < 3; ++channel) {
                weighted[channel] += a_other.weighted[channel];
            }
            weight += a_other.weight;
        }

        // nullopt if the texture is fully transparent
        std::optional<PubesColor::Color> Average() const {
            if (weight <= 0.0) return std::nullopt;
            return PubesColor::Color{static_cast<float>(weighted[0] / weight / 255.0),
                                     static_cast<float>(weighted[1] / weight / 255.0),
                                     static_cast<float>(weighted[2] / weight / 255.0)};
        }
    };

    void AddBlockCompressedLevel(ColorSums& a_sums, const Format a_format, const std::byte* a_level,
                                 const std::uint32_t a_width, const std::uint32_t a_height) {
        const std::uint32_t blocksWide{std::max(1u, (a_width + 3) / 4)};
        const std::uint32_t blocksHigh{std::max(1u, (a_height + 3) / 4)};
        const std::size_t blockSize{GetBlockSize(a_format)};

        // bcdec decodes a 4x4 block to RGBA, 4 bytes per pixel
        std::array<std::uint8_t, 4 * 4 * 4> pixels{};
        constexpr int pitch{4 * 4};

        // The transparent parts of pubes textures are long runs of identical blocks, so a block that's the same as the
        // previous one is skipped
        const std::byte* previousBlock{nullptr};
        std::uint32_t previousPixelsWide{0};
        std::uint32_t previousPixelsHigh{0};
        ColorSums blockSums;

        for (std::uint32_t blockY = 0; blockY < blocksHigh; ++blockY) {
            for (std::uint32_t blockX = 0; blockX < blocksWide; ++blockX) {
                const auto* const block{a_level + (static_cast<std::size_t>(blockY) * blocksWide + blockX) * blockSize};

                // Mip levels smaller than 4x4 still take a whole block, only some of its pixels are part of the image
                const std::uint32_t pixelsWide{std::min(4u, a_width - blockX * 4)};
                const std::uint32_t pixelsHigh{std::min(4u, a_height - blockY * 4)};

                if (previousBlock && pixelsWide == previousPixelsWide && pixelsHigh == previousPixelsHigh &&
                    std::memcmp(block, previousBlock, blockSize) == 0) {
                    a_sums.Add(blockSums);
                    continue;
                }

                switch (a_format) {
                    case Format::BC1:
                        bcdec_bc1(block, pixels.data(), pitch);
                        break;
                    case Format::BC2:
                        bcdec_bc2(block, pixels.data(), pitch);
                        break;
                    case Format::BC3:
                        bcdec_bc3(block, pixels.data(), pitch);
                        break;
                    default:
                        bcdec_bc7(block, pixels.data(), pitch);
                        break;
                }

                blockSums = {};
                for (std::uint32_t y = 0; y < pixelsHigh; ++y) {
                    for (std::uint32_t x = 0; x < pixelsWide; ++x) {
                        const auto* const pixel{pixels.data() + y * pitch + x * 4};
                        blockSums.Add(pixel[0], pixel[1], pixel[2], pixel[3]);
                    }
                }
                a_sums.Add(blockSums);

                previousBlock = block;
                previousPixelsWide = pixelsWide;
                previousPixelsHigh = pixelsHigh;
            }
        }
    }

    void AddUncompressedLevel(ColorSums& a_sums, const Format a_format, const std::byte* a_level,
                              const std::uint32_t a_width, const std::uint32_t a_height) {
        const auto* const bytes{reinterpret_cast<const std::uint8_t*>(a_level)};
        const std::size_t pixelCount{static_cast<std::size_t>(a_width) * a_height};

        for (std::size_t i = 0; i < pixelCount; ++i) {
            const auto* const pixel{bytes + i * 4};
            switch (a_format) {
                case Format::RGBA8:
                    a_sums.Add(pixel[0], pixel[1], pixel[2], pixel[3]);
                    break;
                case Format::BGRA8:
                    a_sums.Add(pixel[2], pixel[1], pixel[0], pixel[3]);
                    break;
                default:
                    a_sums.Add(pixel[2], pixel[1], pixel[0], 255);
                    break;
            }
        }
    }

    std::optional<PubesColor::Color> ComputeAverageColor(const std::string& a_path) {
        const auto start{std::chrono::steady_clock::now()};

        // Goes through the game's file system, so textures packed in BSAs are read too
        RE::BSResourceNiBinaryStream stream{a_path};
        if (!stream.good()) {
            logger::warn("Couldn't open texture {}", a_path);
            return std::nullopt;
        }

        std::uint32_t magic{0};
        DDSHeader header{};
        if (!ReadInto(stream, magic) || magic != DDSMagic || !ReadInto(stream, header) ||
            header.size != sizeof(DDSHeader)) {
            logger::warn("Texture {} isn't a valid DDS file", a_path);
            return std::nullopt;
        }

        std::optional<Format> format;

        if ((header.pixelFormat.flags & DDPFFourCC) && header.pixelFormat.fourCC == MakeFourCC("DX10")) {
            DDSHeaderDX10 headerDX10{};
            if (!ReadInto(stream, headerDX10)) {
                logger::warn("Texture {} isn't a valid DDS file", a_path);
                return std::nullopt;
            }
            format = GetDX10Format(headerDX10.dxgiFormat);
            if (!format) {
                logger::warn("Texture {} has an unsupported format (DXGI format {})", a_path, headerDX10.dxgiFormat);
                return std::nullopt;
            }
        } else {
            format = GetLegacyFormat(header.pixelFormat);
            if (!format) {
                logger::warn("Texture {} has an unsupported format (flags {:#x}, FourCC {:#010x}, {} bits)", a_path,
                             header.pixelFormat.flags, header.pixelFormat.fourCC, header.pixelFormat.rgbBitCount);
                return std::nullopt;
            }
        }

        // The full size image, which comes first. The tools that make the mip levels average the colors of the
        // transparent pixels too, which are usually black, so in the mip levels of sparse textures the hair is much
        // darker than it looks in game
        // Skyrim's textures are at most 16384 pixels wide, anything bigger is a broken header
        if (header.width > 16384 || header.height > 16384) {
            logger::warn("Texture {} has an invalid size of {}x{}", a_path, header.width, header.height);
            return std::nullopt;
        }

        const std::uint32_t width{std::max(1u, header.width)};
        const std::uint32_t height{std::max(1u, header.height)};

        std::vector<std::byte> data(GetLevelSize(*format, width, height));
        if (!stream.read(data.data(), static_cast<std::uint32_t>(data.size()))) {
            logger::warn("Texture {} is smaller than its header says", a_path);
            return std::nullopt;
        }

        ColorSums sums;
        if (IsBlockCompressed(*format)) {
            AddBlockCompressedLevel(sums, *format, data.data(), width, height);
        } else {
            AddUncompressedLevel(sums, *format, data.data(), width, height);
        }

        const auto average{sums.Average()};
        if (average) {
            const std::chrono::duration<double, std::milli> elapsed{std::chrono::steady_clock::now() - start};
            logger::info("Texture {} ({}, {}x{}): average color R {:.0f} G {:.0f} B {:.0f}, read in {:.0f} ms", a_path,
                         FormatName(*format), width, height, (*average)[0] * 255.0f, (*average)[1] * 255.0f,
                         (*average)[2] * 255.0f, elapsed.count());
        } else {
            logger::warn("Texture {} is fully transparent", a_path);
        }
        return average;
    }

    std::mutex g_cacheLock;
    std::unordered_map<std::string, std::optional<PubesColor::Color>> g_cache;

    bool IsCached(const std::string& a_path) {
        std::lock_guard lock{g_cacheLock};
        return g_cache.contains(a_path);
    }

    // One background thread reads the textures queued by CacheAverageColors, one after the other
    std::mutex g_queueLock;
    std::condition_variable g_queueCondition;
    std::deque<std::string> g_queue;
    bool g_workerStarted{false};

    void Worker() {
        while (true) {
            std::string texture;
            {
                std::unique_lock lock{g_queueLock};
                g_queueCondition.wait(lock, [] { return !g_queue.empty(); });
                texture = std::move(g_queue.front());
                g_queue.pop_front();
            }
            PubesColor::GetAverageColor(texture);
        }
    }

    std::optional<PubesColor::Color> GetCachedAverageColor(const std::string_view a_texture) {
        {
            std::lock_guard lock{g_cacheLock};
            if (const auto it{g_cache.find(PubesColor::NormalizePath(a_texture))}; it != g_cache.end()) {
                return it->second;
            }
        }

        logger::info("Texture {} wasn't read yet, reading it in the background", a_texture);
        PubesColor::CacheAverageColors({std::string{a_texture}});
        return std::nullopt;
    }

    struct Hair {
        std::string texture;
        PubesColor::Color tint;
    };

    // The shape of the actor's hair headpart, which is the one tinted by the hair color. Its node in the actor's 3D
    // is named after the headpart's editor ID
    std::optional<Hair> FindHair(RE::Actor* a_actor) {
        auto* const actorBase{a_actor->GetActorBase()};
        auto* const root{a_actor->Get3D(false)};
        if (!actorBase || !root) return std::nullopt;

        const auto* const headPart{actorBase->GetCurrentHeadPartByType(RE::BGSHeadPart::HeadPartType::kHair)};
        if (!headPart || headPart->formEditorID.empty()) return std::nullopt;

        auto* const hairNode{root->GetObjectByName(headPart->formEditorID)};
        if (!hairNode) return std::nullopt;

        std::optional<Hair> hair;
        RE::BSVisit::TraverseScenegraphGeometries(hairNode, [&](RE::BSGeometry* a_geometry) {
            auto* const shader{a_geometry->lightingShaderProp_cast()};
            auto* const material{shader ? shader->material : nullptr};
            if (!material || material->GetFeature() != RE::BSShaderMaterial::Feature::kHairTint) {
                return RE::BSVisit::BSVisitControl::kContinue;
            }

            const auto* const hairMaterial{static_cast<RE::BSLightingShaderMaterialHairTint*>(material)};
            if (!hairMaterial->textureSet) return RE::BSVisit::BSVisitControl::kContinue;

            const char* const texture{hairMaterial->textureSet->GetTexturePath(RE::BSTextureSet::Texture::kDiffuse)};
            if (!texture || !*texture) return RE::BSVisit::BSVisitControl::kContinue;

            const auto& tint{hairMaterial->tintColor};
            hair = Hair{texture, PubesColor::Color{tint.red, tint.green, tint.blue}};
            return RE::BSVisit::BSVisitControl::kStop;
        });

        return hair;
    }
}  // namespace

// BSResource paths are relative to the Data folder, while OPubes' JSON files start them with "data/" and the texture
// paths in meshes may leave out "textures\"
std::string PubesColor::NormalizePath(const std::string_view a_texture) {
    std::string path{a_texture};
    std::ranges::replace(path, '/', '\\');
    std::ranges::transform(path, path.begin(),
                           [](const unsigned char a_char) { return static_cast<char>(std::tolower(a_char)); });
    if (path.starts_with("data\\")) {
        path.erase(0, 5);
    }
    if (!path.starts_with("textures\\")) {
        path.insert(0, "textures\\");
    }
    return path;
}

std::optional<PubesColor::Color> PubesColor::GetAverageColor(const std::string_view a_texture) {
    const auto path{NormalizePath(a_texture)};

    {
        std::lock_guard lock{g_cacheLock};
        if (const auto it{g_cache.find(path)}; it != g_cache.end()) {
            return it->second;
        }
    }

    // Read outside the lock, so a texture being read doesn't block the others. If two threads read the same one,
    // they get the same result and the second one just isn't stored
    const auto color{ComputeAverageColor(path)};

    std::lock_guard lock{g_cacheLock};
    g_cache.emplace(path, color);
    return color;
}

void PubesColor::CacheAverageColors(const std::vector<std::string>& a_textures) {
    {
        std::lock_guard lock{g_queueLock};

        for (const auto& texture : a_textures) {
            if (!IsCached(NormalizePath(texture))) {
                g_queue.push_back(texture);
            }
        }

        if (!g_workerStarted) {
            g_workerStarted = true;
            std::thread(Worker).detach();
        }
    }

    g_queueCondition.notify_one();
}

void PubesColor::CacheHairTexture(RE::Actor* a_actor) {
    if (const auto hair{FindHair(a_actor)}) {
        CacheAverageColors({hair->texture});
    }
}

std::int32_t PubesColor::GetHairColor(RE::Actor* a_actor) {
    const auto hair{FindHair(a_actor)};
    if (!hair) return -1;

    const auto average{GetCachedAverageColor(hair->texture)};
    if (!average) return -1;

    // Skyrim's hair shader multiplies the hair texture by the tint, as far as the hair's vertex colors let the tint
    // through, which most hair has fully
    std::uint32_t color{0};
    for (std::size_t channel = 0; channel < 3; ++channel) {
        const float value{std::clamp((*average)[channel] * hair->tint[channel], 0.0f, 1.0f)};
        color = color << 8 | static_cast<std::uint32_t>(std::lround(value * 255.0f));
    }

    logger::debug("Hair of {} ({:08X}): texture {} tinted R {:.0f} G {:.0f} B {:.0f} shows as {:06X}",
                  a_actor->GetDisplayFullName(), a_actor->GetFormID(), hair->texture, hair->tint[0] * 255.0f,
                  hair->tint[1] * 255.0f, hair->tint[2] * 255.0f, color);
    return static_cast<std::int32_t>(color);
}

std::int32_t PubesColor::GetTintForColor(const std::string_view a_texture, const std::uint32_t a_targetColor) {
    const auto average{GetCachedAverageColor(a_texture)};
    if (!average) return -1;

    // The tint goes through Skyrim's skin tint shader (Racemenu turns overlays into it to tint them), which per channel
    // gives scale * (texture² + 2 * tint * texture * (1 - texture)). A tint of 0.5 leaves the texture as it is,
    // above that it brightens it and below that it darkens it. Solving that for the tint gives the one that turns the
    // texture's average color into the target color, as far as the 0 to 1 range of the tint allows
    constexpr std::array shaderScale{1.01171875f, 0.99609375f, 1.01171875f};
    std::array target{static_cast<float>((a_targetColor >> 16) & 0xFF) / 255.0f,
                      static_cast<float>((a_targetColor >> 8) & 0xFF) / 255.0f,
                      static_cast<float>(a_targetColor & 0xFF) / 255.0f};

    // At most a tint of 1 brightens a channel to scale * (2 * texture - texture²). If the color is brighter than that,
    // like bright red hair with the dark hairy textures, clamping each channel on its own makes the tint white, which
    // leaves the texture's own brown. Darkening the whole color until it fits keeps its hue instead
    float fit{1.0f};
    for (std::size_t channel = 0; channel < 3; ++channel) {
        const float texture{(*average)[channel]};
        if (2.0f * texture * (1.0f - texture) <= 0.0001f) continue;

        if (const float brightest{shaderScale[channel] * (2.0f * texture - texture * texture)};
            target[channel] > brightest) {
            fit = std::min(fit, brightest / target[channel]);
        }
    }
    for (auto& channel : target) {
        channel *= fit;
    }

    std::uint32_t tint{0};
    for (std::size_t channel = 0; channel < 3; ++channel) {
        const float texture{(*average)[channel]};

        // A fully black or white texture can't be changed by the tint, so it's left neutral
        float value{0.5f};
        if (const float range{2.0f * texture * (1.0f - texture)}; range > 0.0001f) {
            value = std::clamp((target[channel] / shaderScale[channel] - texture * texture) / range, 0.0f, 1.0f);
        }

        tint = tint << 8 | static_cast<std::uint32_t>(std::lround(value * 255.0f));
    }

    logger::debug("Tint for texture {} and color {:06X} (darkened to {:.0f}% to fit): {:06X}", a_texture,
                  a_targetColor & 0xFFFFFF, fit * 100.0f, tint);
    return static_cast<std::int32_t>(tint);
}
