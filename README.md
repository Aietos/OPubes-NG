# OPubes

 
Addon for OBody that distributes female pubic hair textures from a number of packs. You can easily create patches in the form of JSON files too.

Download the entire repo to desktop then take "OPubes download me to install in mo.zip" and feed it into your mod organizer

This mod is licensed under GPL-3. This mod may of course be forked and continued. If uploaded to Nexus, out of respect towards my creative & time investment I kindly ask that you point 75% of the DP it earns to my Nexus account (Sairion350). 


Desc



After many delays, OPubes is finally here.

OPubes collects pubic hair textures from disk and categorizes them into 3 groups (hairy, normal, stylized) and then hooks into OBody to distribute them based on an algorithm.


2D Overlays
OPubes pubes are 2D. 3D pubic hairs for SoS exist, but these 2D textures have a few advantages:
- better compatibility
- more variety

With both default pubic hair packs installed, OPubes has a pool of 30 different textures it can use.
Uses external textures
OPubes grabs textures from pubic hair packs you have installed. It does this with simple compatibility JSON files. OPubes ships with compatiblity with 2 known pubic hair texture packs, and you must download these separately from their original pages (links on DL page). You can easily add support for other packs by copying the OPubes json file and editing them.

Smart application rules
OPubes follows a couple of rules for deciding who gets what kind of pubic hair setup. Firstly, around a third of NPCs will be completely shaved. Then, bandits, forsworn, and orcs are given an extra large chance of getting really overgrown pubic hair. Vampires and the like have an extra high chance of stylized pubic hair. Everyone else has a more balanced chance of getting either "normal" or "overgrown" sets, with a rare chance of getting a "stylized" texture.

Works with OBody for the best experience
OPubes uses OBody's skse to work without cloak spells or any spells at all. It's designed to be as high performance as possible.

Additionally, a new texture is chosen when you manually do an OBody application.

Recoloring
OPubes will sometimes slightly tint the textures to match the NPC's hair when possible

Player
The player is skipped by OPubes, you can manually set your pube texture in racemenu's body overlays area

Requirements
OPubes does not ship with any textures. You must download an overlay texture set. Links to all supported sets are available on the download page
OPubes requires OBody

Make sure you aren't using a skin texture with pubes painted on them, obviously.