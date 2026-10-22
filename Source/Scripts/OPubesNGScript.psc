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

Int property SlotToUse auto

bool property EnableMales auto

Message property ChangePubesMessage auto

int Normal
int Stylish
int Hairy

int NormalMale
int StylishMale
int HairyMale

string OPubeskey = "opu"
string OPubesSlot = "op_slt"

int TotalPubesFemale
int TotalPubesMale

bool OStimInstalled


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

	OnLoad()

	debug.notification("OPubes NG installed")
EndEvent


Function OnLoad()
	Console("Game loaded...")

	UnregisterForAllModEvents()

	GeneratePubicHairLists()

	OBodyNative.RegisterForOBodyRemovingClothesEvent(Self as Quest)
	RegisterForModEvent("obody_manualchange", "OnManualChange")

	OStimInstalled = Game.IsPluginInstalled("OStim.esp")
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

	if actorChanged != PlayerRef && (IsFemale(actorChanged) || EnableMales)
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
		RemovePubes(Act, IsFemale(Act))
	endif

	Process(Act, PubesTexture, IgnorePlayer)
EndFunction


Function UpdateActorPubes(Actor Act, String PubesType)
	string texture

	bool isActFemale = IsFemale(Act)

	If PubesType != "Shaved"
		texture = GetPubeTextForType(PubesType, isActFemale)

		if texture != ""
			ChangeActorPubes(Act, texture, false)
		endif
	else
		RemovePubes(Act, isActFemale)
		StoreNPCDataBool(Act, OPubeskey, true)
	endif
EndFunction


Function Process(actor Act, String PubesTexture = "", Bool IgnorePlayer = True)
	bool isActFemale = IsFemale(Act)

	if (isActFemale && TotalPubesFemale > 0) || (EnableMales && TotalPubesMale > 0)
		if GetNPCDataBool(Act, OPubesKey)
			; this if is needed to not mess up overlays added by other mods
			if !NiOverride.HasOverlays(Act) && (!OStimInstalled || !OActor.IsInOStim(Act))
				; this is needed so that overlays stay between save reloads
				NiOverride.AddOverlays(Act)
			endif
		elseif (Act != PlayerRef || (Act == PlayerRef && !IgnorePlayer))
			StoreNPCDataBool(Act, OPubeskey, true)

			string texture

			if PubesTexture == ""
				texture = GetPubeTexForNPC(Act, isActFemale)
			else
				texture = PubesTexture
			endif

			if texture != ""
				ApplyOverlay(Act, texture, isActFemale)

				if ChanceRoll(75)
					ColorPubes(Act, GetLightHairColor(Act), isActFemale)
				else
					ColorPubes(Act, ColorComponent.SetValue(0x0, Utility.RandomFloat(0.06, 0.38)), isActFemale)
				endif
			endif
		endif
	endif
EndFunction


String Function GetPubeTextForType(String PubesType, bool isActFemale)
	int normalArray = Normal
	int stylishArray = Stylish
	int hairyArray = Hairy

	if !IsActFemale
		normalArray = NormalMale
		stylishArray = StylishMale
		hairyArray = HairyMale
	endif

	int normalLength = JArray.count(normalArray)
	int stylishLength = JArray.count(stylishArray)
	int hairyLength = JArray.count(hairyArray)

	if PubesType == "Normal" && normalLength > 0
		return JArray.getStr(normalArray, Utility.RandomInt(0, normalLength - 1))
	elseif PubesType == "Stylish" && stylishLength > 0
		return JArray.getStr(stylishArray, Utility.RandomInt(0, stylishLength - 1))
	elseif PubesType == "Hairy" && hairyLength > 0
		return JArray.getStr(hairyArray, Utility.RandomInt(0, hairyLength - 1))
	endif

	return ""
EndFunction


string Function GetPubeTexForNPC(actor npc, bool isActFemale)
	int type = GetHairType(npc, isActFemale)

	if type != -1
		int typeLength = JArray.count(type)

		if typeLength > 0
			return JArray.getStr(type, Utility.RandomInt(0, typeLength - 1))
		endif
	endif

	return ""
EndFunction


int Function GetHairType(Actor Npc, bool IsActFemale)
	Race npcRace = Npc.GetLeveledActorBase().GetRace()

	if npcRace.HasKeyword(IsBeastRace)
		return -1
	endif

	int normalArray = Normal
	int stylishArray = Stylish
	int hairyArray = Hairy

	if !IsActFemale
		normalArray = NormalMale
		stylishArray = StylishMale
		hairyArray = HairyMale
	endif

	if StringContains(npcRace.GetName(), "orc")
		if ChanceRoll(75)
			return hairyArray
		elseif ChanceRoll(50)
			return normalArray
		elseif ChanceRoll(15)
			return stylishArray
		else
			return -1
		endif
	endif

	if npcRace.HasKeyword(vampire)
		if chanceRoll(50)
			return stylishArray
		elseif ChanceRoll(30)
			return -1
		else
			return normalArray
		endif
	elseif npc.IsInFaction(Banditfaction) || npc.IsInFaction(Forswornfaction)
		if chanceRoll(20)
			return -1
		Else
			if chanceRoll(80)
				return hairyArray
			else
				return normalArray
			endif
		endif
	else ; most NPCs
		int shavedProb = ShavedChance
		int normalProb = ShavedChance + NormalChance
		int stylishProb = ShavedChance + NormalChance + StylishChance
		int hairyProb = ShavedChance + NormalChance + StylishChance + HairyChance

		int randomNum = Utility.RandomInt(1, 100)

		if randomNum <= shavedProb
			return -1
		elseif randomNum <= normalProb
			return normalArray
		elseif randomNum <= stylishProb
			return stylishArray
		elseif randomNum <= hairyProb
			return hairyArray
		endif

		; a default if user messed up and chances don't sum up to 100
		return normalArray
	endif
EndFunction


; ██╗   ██╗████████╗██╗██╗     ███████╗
; ██║   ██║╚══██╔══╝██║██║     ██╔════╝
; ██║   ██║   ██║   ██║██║     ███████╗
; ██║   ██║   ██║   ██║██║     ╚════██║
; ╚██████╔╝   ██║   ██║███████╗███████║
;  ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝


Function RemovePubes(actor Act, bool IsActFemale)
	String Node = "Body [ovl" + GetNPCDataInt(Act, OPubesSlot) + "]"

	NiOverride.AddNodeOverrideString(Act, IsActFemale, Node, 9, 0, "actors\\character\\overlays\\default.dds", false)
	NiOverride.RemoveNodeOverride(Act, IsActFemale, node , 9, 0)
	NiOverride.RemoveNodeOverride(Act, IsActFemale, Node, 7, -1)
	NiOverride.RemoveNodeOverride(Act, IsActFemale, Node, 0, -1)
	NiOverride.RemoveNodeOverride(Act, IsActFemale, Node, 8, -1)
	NiOverride.RemoveNodeOverride(Act, IsActFemale, Node, 2, -1)
	NiOverride.RemoveNodeOverride(Act, IsActFemale, Node, 3, -1)

	StoreNPCDataBool(act, OPubeskey, false)
EndFunction


int Function GetLightHairColor(actor act)
	int hairColor = Act.GetLeveledActorBase().GetHairColor().GetColor()

	float newBrightness = PapyrusUtil.ClampFloat(ColorComponent.GetValue(hairColor) * 2.0, 0.0, 1.0)

	return ColorComponent.setvalue(haircolor, newBrightness)
EndFunction


Function ColorPubes(actor act, int color, bool isActFemale)
	String Node = "Body" + " [ovl" + GetNPCDataInt(act, OPubesSlot) + "]"

	NiOverride.AddNodeOverrideInt(act, isActFemale, Node, 7, -1, color, true)
EndFunction


Function ApplyOverlay(Actor akTarget, String TextureToApply, bool isActFemale)
	int OverlaySlot = GetEmptySlot(aktarget, isActFemale, "Body")
	StoreNPCDataInt(akTarget, OPubesSlot, OverlaySlot)

	String Node = "Body" + " [ovl" + OverlaySlot + "]"

	NiOverride.AddNodeOverrideString(akTarget, isActFemale, Node, 9, 0, TextureToApply, true)
	NiOverride.AddNodeOverrideInt(akTarget, isActFemale, Node, 7, -1, 0, true)
    NiOverride.AddNodeOverrideInt(akTarget, isActFemale, Node, 0, -1, 0, true)
    NiOverride.AddNodeOverrideFloat(akTarget, isActFemale, Node, 8, -1, 1.0, true)
	NiOverride.AddNodeOverrideFloat(akTarget, isActFemale, Node, 2, -1, 0.0, true)
	NiOverride.AddNodeOverrideFloat(akTarget, isActFemale, Node, 3, -1, 0.0, true)

	NiOverride.ApplyNodeOverrides(akTarget)
EndFunction


Int Function GetEmptySlot(Actor akTarget, Bool Gender, String Area)
	String TexPath

	if SlotToUse > 0
		TexPath = NiOverride.GetNodeOverrideString(akTarget, Gender, Area + " [ovl" + SlotToUse + "]", 9, 0)

		If TexPath == "" || TexPath == "actors\\character\\overlays\\default.dds"
			Return SlotToUse
		EndIf
	endif

	Int i = 0
	Int NumSlots = NiOverride.GetNumBodyOverlays()
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


Function GeneratePubicHairLists()
	ClearJArrays()

	Normal = JArray.object()
	JValue.retain(Normal, "OPubesNormal")

	Stylish = JArray.object()
	JValue.retain(Stylish, "OPubesStylish")

	Hairy = JArray.object()
	JValue.retain(Hairy, "OPubesHairy")

	; Parse female pubes
	ParseJSONFiles("data/meshes/opubes", Normal, Stylish, Hairy)

	int FemaleTotalNormal = JArray.count(Normal)
	int FemaleTotalStylish = JArray.count(Stylish)
	int FemaleTotalHairy = JArray.count(Hairy)

	TotalPubesFemale = FemaleTotalNormal + FemaleTotalStylish + FemaleTotalHairy

	TotalPubesMale = 0

	; Parse male pubes
	if EnableMales
		NormalMale = JArray.object()
		JValue.retain(NormalMale, "OPubesNormalMale")

		StylishMale = JArray.object()
		JValue.retain(StylishMale, "OPubesStylishMale")

		HairyMale = JArray.object()
		JValue.retain(HairyMale, "OPubesHairyMale")

		ParseJSONFiles("data/meshes/opubes/male", NormalMale, StylishMale, HairyMale)

		int MaleTotalNormal = JArray.count(NormalMale)
		int MaleTotalStylish = JArray.count(StylishMale)
		int MaleTotalHairy = JArray.count(HairyMale)

		TotalPubesMale = MaleTotalNormal + MaleTotalStylish + MaleTotalHairy
	endif

	Console("Hair counts:")

	Console(">	Total Pubes Female: " + TotalPubesFemale)
	Console(">	Total Pubes Male: " + TotalPubesMale)
EndFunction


Function ParseJSONFiles(String dir, int normalArray, int stylishArray, int hairyArray)
	string[] compatbilityFiles = MiscUtil.FilesInFolder(dir, extension = "json")

	Console(compatbilityFiles.Length + " pubic hair compat files found")

	int i = 0
	int l = compatbilityFiles.Length

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
					JArray.addStr(stylishArray, hairs[j])
				elseif type == "normal"
					JArray.addStr(normalArray, hairs[j])
				elseif type == "hairy"
					JArray.addStr(hairyArray, hairs[j])
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
EndFunction

Function ClearJArrays()
	JArray.Clear(Normal)
	JArray.Clear(Stylish)
	JArray.Clear(Hairy)

	JArray.Clear(NormalMale)
	JArray.Clear(StylishMale)
	JArray.Clear(HairyMale)

	JValue.Release(Normal)
	JValue.Release(Stylish)
	JValue.Release(Hairy)

	JValue.Release(NormalMale)
	JValue.Release(StylishMale)
	JValue.Release(HairyMale)
EndFunction
