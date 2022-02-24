ScriptName OPubesScript Extends quest 
import OUtils

keyword IsBeastRace
keyword Vampire
faction BanditFaction
faction ForswornFaction

actor playerref

Event OnInit()
	outils.GetOStim().RegisterForGameLoadEvent(self)
	outils.RegisterForOUpdate(self)

	playerref = game.GetPlayer()
	IsBeastRace = Keyword.GetKeyword("IsBeastRace")
	BanditFaction = GetFormFromFile(0x1bcc0, "skyrim.esm") as Faction
	ForswornFaction = GetFormFromFile(0x43599, "skyrim.esm") as Faction
	vampire = Keyword.GetKeyword("Vampire")

	stylish = PapyrusUtil.StringArray(0)
	normal = PapyrusUtil.StringArray(0)
	hairy = PapyrusUtil.StringArray(0)
	nohair = PapyrusUtil.StringArray(1, "hairless")

	if obodyscript.get().GetAPIVersion() < 2
		debug.messagebox("OBody is out of date. update to use OPubes")
		return
	endif 
	if outils.GetOStim().GetAPIVersion() < 20
		debug.messagebox("OStim is out of date. update to use OPubes")
		return 
	endif 
	GeneratePubicHairLists()

	OnGameLoad()
	debug.notification("OPubes installed")
EndEvent

string[] stylish
string[] normal
string[] hairy 
string[] nohair

string opubeskey = "opu"
string opubesslot = "op_slt"

Function Process(actor act)
	;Console("processing...")


	if outils.AppearsFemale(act)  && !(GetNPCDataBool(act, opubeskey)) && (act != playerref)
		StoreNPCDataBool(act, opubeskey, true)
		string tex = GetPubeTexForNPC(act) 
		if tex != "None"
			;Console(">	Actor: " + act.GetDisplayName())
			;Console(">	Applying overlay: " + tex)
			ApplyOverlay(act, tex)

			if ChanceRoll(75)
				ColorPubes(act, GetLightHairColor(act))
			else 
				ColorPubes(act, ColorComponent.SetValue(0x0, OSANative.RandomFloat(0.06, 0.38)))
			endif
		endif 
	endif
endfunction 

Event OnActorNaked(Actor act)
	Process(act)
EndEvent

Event OnManualChange(form act)
	if GetNPCDataBool(act as actor, opubeskey)
		RemovePubes(act as actor)
	endif
	Process(act as actor)
EndEvent

Event OnGameLoad()
	OBodyNative.RegisterForOBodyNakedEvent(self as quest)
	RegisterForModEvent("obody_manualchange", "OnManualChange")
EndEvent

Function RemovePubes(actor act)
	NiOverride.RemoveAllNodeNameOverrides(act, true, "Body" + " [ovl" + GetNPCDataInt(act, opubesslot) + "]")
	StoreNPCDataBool(act, opubeskey, false)
EndFunction

function ColorPubes(actor act, int color)
	String Node = "Body" + " [ovl" + GetNPCDataInt(act, opubesslot) + "]"

	NiOverride.AddNodeOverrideInt(act, true, Node, 7, -1, color, true)
endfunction 

string function GetPubeTexForNPC(actor npc)
	string[] type = GetHairType(npc)

	if type == nohair
		return none 
	else 
		return type[OSANative.RandomInt(0, type.Length - 1)]
	endif 
EndFunction

string[] Function GetHairType(actor npc)
	actorbase base = OSANative.GetLeveledActorBase(npc)
	race npcRace = OSANative.GetRace(base)

	if npcRace.HasKeyword(IsBeastRace)
		return nohair
	elseif StringContains(OSANative.GetName(npcRace), "orc")
		if ChanceRoll(75)
			return hairy 
		elseif ChanceRoll(50)
			return normal 
		elseif ChanceRoll(15)
			return stylish
		else
			return nohair
		endif 
	endif 

	if npcrace.haskeyword(vampire)
		if chanceRoll(50)
			return stylish
		elseif ChanceRoll(30)
			return nohair 
		else 
			return normal 
		endif 

	elseif npc.IsInFaction(Banditfaction) || npc.IsInFaction(Forswornfaction) 
		if chanceRoll(20)
			return nohair
		Else
			if chanceRoll(80)
				return hairy 
			else
				return normal
			endif		
		endif

	else ; most NPCs
		if ChanceRoll(30)
			return nohair 
		else 
			if ChanceRoll(50)
				Return normal
			elseif ChanceRoll(75)
				return hairy 
			else 
				return stylish 
			endif 
		endif 

	endif 

endfunction 

int Function GetLightHairColor(actor act)
	int hairColor = OSANative.GetLeveledActorBase(act).GetHairColor().GetColor()
	float newBrightness = PapyrusUtil.ClampFloat(ColorComponent.GetValue(hairColor) * 2.0, 0.0, 1.0)

	return ColorComponent.setvalue(haircolor, newBrightness)
endfunction 


Function ApplyOverlay(Actor akTarget, String TextureToApply)
	int OverlaySlot = GetEmptySlot(aktarget, true, "Body")
		StoreNPCDataInt(akTarget, opubesslot, OverlaySlot)

	NiOverride.AddOverlays(akTarget)
	String Node = "Body" + " [ovl" + OverlaySlot + "]"
	
	NiOverride.AddNodeOverrideString(akTarget, true, Node, 9, 0, TextureToApply, true)
	NiOverride.AddNodeOverrideInt(akTarget, true, Node, 7, -1, 0, true)
    NiOverride.AddNodeOverrideInt(akTarget, true, Node, 0, -1, 0, true)
    NiOverride.AddNodeOverrideFloat(akTarget, true, Node, 8, -1, 1.0, true)
	NiOverride.AddNodeOverrideFloat(akTarget, true, Node, 2, -1, 0.0, true)
	NiOverride.AddNodeOverrideFloat(akTarget, true, Node, 3, -1, 0.0, true)
	
	NiOverride.ApplyNodeOverrides(akTarget)
EndFunction



Function GeneratePubicHairLists()
	string dir = "data/meshes/opubes"
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
					stylish = PapyrusUtil.PushString(stylish, hairs[j])
				elseif type == "normal"
					normal = PapyrusUtil.PushString(normal, hairs[j])
				elseif type == "hairy"
					hairy = PapyrusUtil.PushString(hairy, hairs[j])
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


	Console("OPubes installed. Hair counts:")

	Console(">	Stylish: " + stylish.Length)
	Console(">	Normal: " + normal.Length)
	Console(">	Hairy: " + hairy.Length)

	Console(">	Total: " + (hairy.Length + normal.Length + stylish.Length))
EndFunction




Bool Function AppearsFemale(Actor Act) 
	{gender based / looks like a woman but can have a penis}
	Return OSANative.GetSex(OSANative.GetLeveledActorBase(act)) == 1
EndFunction

Int Function GetEmptySlot(Actor akTarget, Bool Gender, String Area)
	Int i = 0
	Int NumSlots = NiOverride.GetNumBodyOverlays()
	String TexPath
	Bool FirstPass = true

	While i < NumSlots
		TexPath = NiOverride.GetNodeOverrideString(akTarget, Gender, Area + " [ovl" + i + "]", 9, 0)
		If TexPath == "" || TexPath == "actors\\character\\overlays\\default.dds"
			;console("Slot " + i + " chosen for area: " + area)
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
