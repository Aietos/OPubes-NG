#pragma once

namespace PubesColor {
    // Red, green and blue from 0 to 1
    using Color = std::array<float, 3>;

    // The path as BSResource finds it: lowercase, with backslashes, starting at textures\ without data\ in front
    std::string NormalizePath(std::string_view a_texture);

    // The average color of the texture as it shows in game, from its full size image with each pixel weighted by its
    // alpha, so the transparent parts don't count
    // Reads the texture if it isn't cached yet, so it's only called from the background thread
    // nullopt if the texture couldn't be read or its format isn't supported
    std::optional<Color> GetAverageColor(std::string_view a_texture);

    // Reads the textures' average colors in a background thread, so it doesn't happen while NPCs undress
    void CacheAverageColors(const std::vector<std::string>& a_textures);

    // Reads the average color of the actor's hair texture in the background, so it's known when they undress
    // Needs the actor's 3D, so it's called from the main thread
    void CacheHairTexture(RE::Actor* a_actor);

    // The color the actor's hair shows in game (0xRRGGBB), which is its texture tinted by the hair color, or -1 if
    // the hair or its texture couldn't be found, or its texture wasn't read yet. Needs the actor's 3D, so it's called
    // from the main thread
    std::int32_t GetHairColor(RE::Actor* a_actor);

    // The overlay tint (0xRRGGBB) that makes the texture's average color as close as possible to a_targetColor
    // (0xRRGGBB, any alpha in the top byte is ignored), or -1 if the texture couldn't be read or wasn't read yet
    std::int32_t GetTintForColor(std::string_view a_texture, std::uint32_t a_targetColor);
}  // namespace PubesColor
