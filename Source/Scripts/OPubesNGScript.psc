ScriptName OPubesNGScript Extends Quest

Actor property PlayerRef auto

Keyword property IsBeastRace auto
Keyword property Vampire auto

Faction property BanditFaction auto
Faction property ForswornFaction auto

Quest property OPubesOldQuest auto

Int property PubesCycleKey auto

Int property ShavedChance auto
Int property NormalChance auto
Int property StylishChance auto
Int property HairyChance auto

Int property OBodyManualChangeBehaviour auto

Message property ChangePubesMessage auto

string[] Shaved
string[] Normal
string[] Stylish
string[] Hairy

string OPubeskey = "opu"
string OPubesSlot = "op_slt"

int TotalPubes


; ███████╗██╗   ██╗███████╗███╗   ██╗████████╗███████╗
; ██╔════╝██║   ██║██╔════╝████╗  ██║╚══██╔══╝██╔════╝
; █████╗  ██║   ██║█████╗  ██╔██╗ ██║   ██║   ███████╗
; ██╔══╝  ╚██╗ ██╔╝██╔══╝  ██║╚██╗██║   ██║   ╚════██║
; ███████╗ ╚████╔╝ ███████╗██║ ╚████║   ██║   ███████║
; ╚══════╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝


Event OnInit()
	If OPubesOldQuest.IsRunning()
		OPubesOldQuest.Stop()
	endif

	RegisterForKey(PubesCycleKey)

	Stylish = PapyrusUtil.StringArray(0)
	Normal = PapyrusUtil.StringArray(0)
	Hairy = PapyrusUtil.StringArray(0)
	Shaved = PapyrusUtil.StringArray(1, "hairless")

	OnLoad()

	debug.notification("OPubes NG installed")
EndEvent


Function OnLoad()
	Console("Game loaded...")

	UnregisterForAllModEvents()

	GeneratePubicHairLists()

	OBodyNative.RegisterForOBodyRemovingClothesEvent(Self as Quest)
	RegisterForModEvent("obody_manualchange", "OnManualChange")
EndFunction


Event OnKeyDown(int KeyPress)
	if KeyPress == PubesCycleKey && KeyPress != 1
		Actor actorInCrosshair = Game.GetCurrentCrosshairRef() as Actor

		if actorInCrossHair == none
			actorInCrossHair = PlayerRef
		endif

		ChangeActorPubes(actorInCrosshair, "", false)
	endif
EndEvent


Event OnActorRemovingClothes(Actor Act)
	Process(Act)
EndEvent


Event OnManualChange(Form Act)
	Actor actorChanged = Act as Actor

	if actorChanged != PlayerRef && IsFemale(actorChanged)
		if OBodyManualChangeBehaviour == 0
			return
		elseif OBodyManualChangeBehaviour == 1
			ChangeActorPubes(actorChanged)
		else
			int result = ChangePubesMessage.Show()

			if result == 0
				ChangeActorPubes(actorChanged)
			endif
		endif
	endif
EndEvent


Function UpdatePubesCycleKey(int previousKey)
	UnregisterForKey(PreviousKey)
	RegisterForKey(PubesCycleKey)
EndFunction


; ███╗   ███╗ █████╗ ██╗███╗   ██╗
; ████╗ ████║██╔══██╗██║████╗  ██║
; ██╔████╔██║███████║██║██╔██╗ ██║
; ██║╚██╔╝██║██╔══██║██║██║╚██╗██║
; ██║ ╚═╝ ██║██║  ██║██║██║ ╚████║
; ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝


Function ChangeActorPubes(Actor Act, String PubesTexture = "", Bool IgnorePlayer = True)
	if GetNPCDataBool(Act, OPubeskey)
		RemovePubes(Act)
	endif

	Process(Act, PubesTexture, IgnorePlayer)
EndFunction


Function UpdateActorPubes(Actor Act, String PubesType)
	string texture

	If PubesType != "Shaved"
		texture = GetPubeTextForType(PubesType)

		if texture != ""
			ChangeActorPubes(Act, texture, false)
		endif
	else
		RemovePubes(Act)
		StoreNPCDataBool(Act, OPubeskey, true)
	endif
EndFunction


Function Process(actor Act, String PubesTexture = "", Bool IgnorePlayer = True)
	if IsFemale(Act)
		if GetNPCDataBool(Act, OPubesKey)
			NiOverride.AddOverlays(Act)
		elseif (Act != PlayerRef || (Act == PlayerRef && !IgnorePlayer))
			StoreNPCDataBool(Act, OPubeskey, true)

			string texture

			if PubesTexture == ""
				texture = GetPubeTexForNPC(Act)
			else
				texture = PubesTexture
			endif

			if texture != ""
				ApplyOverlay(Act, texture)

				if ChanceRoll(75)
					ColorPubes(Act, GetLightHairColor(Act))
				else
					ColorPubes(Act, ColorComponent.SetValue(0x0, Utility.RandomFloat(0.06, 0.38)))
				endif
			endif
		endif
	endif
EndFunction


String Function GetPubeTextForType(String PubesType)
	if PubesType == "Normal" && Normal.length > 0
		return Normal[Utility.RandomInt(0, Normal.Length - 1)]
	elseif PubesType == "Stylish" && Stylish.length > 0
		return Stylish[Utility.RandomInt(0, Stylish.Length - 1)]
	elseif PubesType == "Hairy" && Hairy.length > 0
		return Hairy[Utility.RandomInt(0, Hairy.Length - 1)]
	endif

	return ""
EndFunction


string Function GetPubeTexForNPC(actor npc)
	string[] type = GetHairType(npc)

	if type == Shaved || type.length == 0
		return ""
	else
		return type[Utility.RandomInt(0, type.Length - 1)]
	endif
EndFunction


string[] Function GetHairType(Actor Npc)
	Race npcRace = Npc.GetLeveledActorBase().GetRace()

	if npcRace.HasKeyword(IsBeastRace)
		return Shaved
	endif

	if StringContains(npcRace.GetName(), "orc")
		if ChanceRoll(75)
			return Hairy
		elseif ChanceRoll(50)
			return Normal
		elseif ChanceRoll(15)
			return Stylish
		else
			return Shaved
		endif
	endif

	if npcRace.HasKeyword(vampire)
		if chanceRoll(50)
			return Stylish
		elseif ChanceRoll(30)
			return Shaved
		else
			return Normal
		endif
	elseif npc.IsInFaction(Banditfaction) || npc.IsInFaction(Forswornfaction)
		if chanceRoll(20)
			return Shaved
		Else
			if chanceRoll(80)
				return Hairy
			else
				return Normal
			endif
		endif
	else ; most NPCs
		int shavedProb = ShavedChance
		int normalProb = ShavedChance + NormalChance
		int stylishProb = ShavedChance + NormalChance + StylishChance
		int hairyProb = ShavedChance + NormalChance + StylishChance + HairyChance

		int randomNum = Utility.RandomInt(1, 100)

		if randomNum <= shavedProb
			return Shaved
		elseif randomNum <= normalProb
			return Normal
		elseif randomNum <= stylishProb
			return Stylish
		elseif randomNum <= hairyProb
			return Hairy
		endif

		; a default if user messed up and chances don't sum up to 100
		return Normal
	endif
EndFunction


; ██╗   ██╗████████╗██╗██╗     ███████╗
; ██║   ██║╚══██╔══╝██║██║     ██╔════╝
; ██║   ██║   ██║   ██║██║     ███████╗
; ██║   ██║   ██║   ██║██║     ╚════██║
; ╚██████╔╝   ██║   ██║███████╗███████║
;  ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝


Function RemovePubes(actor Act)
	String Node = "Body [ovl" + GetNPCDataInt(Act, OPubesSlot) + "]"

	NiOverride.AddNodeOverrideString(Act, true, Node, 9, 0, "actors\\character\\overlays\\default.dds", false)
	NiOverride.RemoveNodeOverride(Act, true, node , 9, 0)
	NiOverride.RemoveNodeOverride(Act, true, Node, 7, -1)
	NiOverride.RemoveNodeOverride(Act, true, Node, 0, -1)
	NiOverride.RemoveNodeOverride(Act, true, Node, 8, -1)
	NiOverride.RemoveNodeOverride(Act, true, Node, 2, -1)
	NiOverride.RemoveNodeOverride(Act, true, Node, 3, -1)

	StoreNPCDataBool(act, OPubeskey, false)
EndFunction


int Function GetLightHairColor(actor act)
	int hairColor = Act.GetLeveledActorBase().GetHairColor().GetColor()

	float newBrightness = PapyrusUtil.ClampFloat(ColorComponent.GetValue(hairColor) * 2.0, 0.0, 1.0)

	return ColorComponent.setvalue(haircolor, newBrightness)
EndFunction


Function ColorPubes(actor act, int color)
	String Node = "Body" + " [ovl" + GetNPCDataInt(act, OPubesSlot) + "]"

	NiOverride.AddNodeOverrideInt(act, true, Node, 7, -1, color, true)
EndFunction


Function ApplyOverlay(Actor akTarget, String TextureToApply)
	int OverlaySlot = GetEmptySlot(aktarget, true, "Body")
	StoreNPCDataInt(akTarget, OPubesSlot, OverlaySlot)

	String Node = "Body" + " [ovl" + OverlaySlot + "]"

	NiOverride.AddNodeOverrideString(akTarget, true, Node, 9, 0, TextureToApply, true)
	NiOverride.AddNodeOverrideInt(akTarget, true, Node, 7, -1, 0, true)
    NiOverride.AddNodeOverrideInt(akTarget, true, Node, 0, -1, 0, true)
    NiOverride.AddNodeOverrideFloat(akTarget, true, Node, 8, -1, 1.0, true)
	NiOverride.AddNodeOverrideFloat(akTarget, true, Node, 2, -1, 0.0, true)
	NiOverride.AddNodeOverrideFloat(akTarget, true, Node, 3, -1, 0.0, true)

	NiOverride.AddOverlays(akTarget)

	NiOverride.ApplyNodeOverrides(akTarget)
EndFunction


Function GeneratePubicHairLists()
	string dir = "data/meshes/opubes"
	string[] compatbilityFiles = MiscUtil.FilesInFolder(dir, extension = "json")

	Console(compatbilityFiles.Length + " pubic hair compat files found")

	int i = 0
	int l = compatbilityFiles.Length

	Shaved = Utility.ResizeStringArray(Shaved, 0, "")
	Normal = Utility.ResizeStringArray(Normal, 0, "")
	Stylish = Utility.ResizeStringArray(Stylish, 0, "")
	Hairy = Utility.ResizeStringArray(Hairy, 0, "")

	while i < l
		int file = JValue.readFromFile(dir + "/" + compatbilityFiles[i])

		string[] hairs = jmap.allKeysPArray(file)
		Console(hairs.Length + " pubic hairs in " + compatbilityFiles[i])

		int j = 0
		int l2 = hairs.Length
		while j < l2
			if MiscUtil.FileExists(hairs[j])
				string type = jmap.getStr(file, hairs[j])

				if type == "stylish"
					Stylish = PapyrusUtil.PushString(Stylish, hairs[j])
				elseif type == "normal"
					Normal = PapyrusUtil.PushString(Normal, hairs[j])
				elseif type == "hairy"
					Hairy = PapyrusUtil.PushString(Hairy, hairs[j])
				else
					Console("OPUBES ERROR: Type for hair " + hairs[j] + " not found: " + type)
				endif
			else
				Console("OPubes - file not found: " + hairs[j])
				Console("OPubes - Assuming that this pack is not installed and skipping")
				j = l2
			endif

			j += 1
		endwhile

		i += 1
	endwhile


	Console("Hair counts:")

	Console(">	Stylish: " + Stylish.Length)
	Console(">	Normal: " + Normal.Length)
	Console(">	Hairy: " + Hairy.Length)

	TotalPubes = Hairy.Length + Normal.Length + Stylish.Length

	Console(">	Total: " + TotalPubes)
EndFunction


Int Function GetEmptySlot(Actor akTarget, Bool Gender, String Area)
	Int i = 0
	Int NumSlots = NiOverride.GetNumBodyOverlays()
	String TexPath
	Bool FirstPass = true

	While i < NumSlots
		TexPath = NiOverride.GetNodeOverrideString(akTarget, Gender, Area + " [ovl" + i + "]", 9, 0)
		If TexPath == "" || TexPath == "actors\\character\\overlays\\default.dds"
			Return i
		EndIf
		i += 1
		If !FirstPass && i == NumSlots
			FirstPass = true
			i = 0
		EndIf
	EndWhile
	Return -1
EndFunction


Function Console(String In)
	MiscUtil.PrintConsole("OPubes NG: " + In)
EndFunction


Function StoreNPCDataInt(actor npc, string keys, int num)
	StorageUtil.SetIntValue(npc as form, keys, num)
EndFunction


Function StoreNPCDataBool(actor npc, string keys, bool value)
	int store

	if value
		store = 1
	else
		store = 0
	endif

	StoreNPCDataInt(npc, keys, store)
EndFunction


Int Function GetNPCDataInt(actor npc, string keys)
	return StorageUtil.GetIntValue(npc, keys, -1)
EndFunction


Bool Function GetNPCDataBool(actor npc, string keys)
	int value = GetNPCDataInt(npc, keys)
	bool ret = (value == 1)
	return ret
EndFunction


Bool Function ChanceRoll(Int Chance) ; input 60: 60% of returning true
	return ( (Utility.RandomInt(0, 99) ) < Chance)
EndFunction


Bool Function StringContains(string str, string contains)
	return (StringUtil.Find(str, contains) != -1)
EndFunction


Bool Function IsFemale(Actor Act)
	return Act.GetLeveledActorBase().GetSex() == 1
EndFunction
