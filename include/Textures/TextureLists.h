#pragma once

namespace TextureLists {
    // The pubes textures OPubes' JSON files list, with the type each one is
    struct Lists {
        std::vector<std::string> normal;
        std::vector<std::string> stylish;
        std::vector<std::string> hairy;

        [[nodiscard]] bool empty() const { return normal.empty() && stylish.empty() && hairy.empty(); }
    };

    // Reads the female and male lists and has their textures' colors read in the background in the main menu
    // Called once when the data is loaded, the lists don't change after that
    void Load();

    const Lists& GetFemale();
    const Lists& GetMale();

    // If the texture is one of the listed ones, to recognize the pubes in saves from before the DLL kept track of them
    bool IsPubesTexture(std::string_view a_texture);
}  // namespace TextureLists
