Scriptname OPubesNGMCMScript extends SKI_ConfigBase

; Settings
int setOBodyManualChangeBehaviour
int setShavedChance
int setNormalChance
int setStylishChance
int setHairyChance
int setPubeCycleKey
int setResetBodyDistribution
int setPubeActor

string[] pubeTypeList
string[] obodyManualChangeBehaviours

OPubesNGScript property OPubes auto


event OnInit()
	parent.OnInit()

	Modname = "OPubes NG"
endEvent


event OnGameReload()
	parent.onGameReload()

	pubeTypeList = new string[5]
	pubeTypeList[0] = "Select Pube Type"
	pubeTypeList[1] = "Shaved"
	pubeTypeList[2] = "Normal"
	pubeTypeList[3] = "Stylish"
	pubeTypeList[4] = "Hairy"

	obodyManualChangeBehaviours = new string[3]
	obodyManualChangeBehaviours[0] = "Don't change pubes"
	obodyManualChangeBehaviours[1] = "Randomize pubes"
	obodyManualChangeBehaviours[2] = "Ask before changing pubes"
endevent


event OnPageReset(string page)
	SetCursorFillMode(TOP_TO_BOTTOM)

	setOBodyManualChangeBehaviour = AddMenuOption("$opubes_option_obody_manual_behaviour", obodyManualChangeBehaviours[OPubes.OBodyManualChangeBehaviour])

	setShavedChance = AddSliderOption("$opubes_option_shaved_chance", OPubes.ShavedChance)
	setNormalChance = AddSliderOption("$opubes_option_normal_chance", OPubes.NormalChance)
	setStylishChance = AddSliderOption("$opubes_option_stylish_chance", OPubes.StylishChance)
	setHairyChance = AddSliderOption("$opubes_option_hairy_chance", OPubes.HairyChance)

	AddEmptyOption()

	setPubeCycleKey = AddKeyMapOption("$opubes_option_pube_cycle_key", OPubes.PubesCycleKey)

	AddEmptyOption()

	Actor actorInCrosshair = Game.GetCurrentCrosshairRef() as Actor

	if actorInCrosshair == none
		if OPubes.IsFemale(OPubes.PlayerRef)
			actorInCrosshair = OPubes.PlayerRef
		endif
	endif

	if actorInCrossHair != none
		Race actorRace = actorInCrosshair.GetLeveledActorBase().GetRace()

		if OPubes.IsFemale(actorInCrossHair) && !actorRace.HasKeyword(OPubes.IsBeastRace)
			setPubeActor = AddMenuOption(actorInCrosshair.GetActorBase().GetName(), pubeTypeList[0])
		endif
	endif
endEvent


Event OnOptionSliderOpen(int option)
	if (option == setShavedChance)
		SetSliderDialogStartValue(OPubes.ShavedChance)
		SetSliderDialogDefaultValue(25.0)
		SetSliderDialogRange(0.0, 100.0)
		SetSliderDialogInterval(1.0)

	elseif (option == setNormalChance)
		SetSliderDialogStartValue(OPubes.NormalChance)
		SetSliderDialogDefaultValue(25.0)
		SetSliderDialogRange(0.0, 100.0)
		SetSliderDialogInterval(1.0)

	elseif (option == setStylishChance)
		SetSliderDialogStartValue(OPubes.StylishChance)
		SetSliderDialogDefaultValue(25.0)
		SetSliderDialogRange(0.0, 100.0)
		SetSliderDialogInterval(1.0)

	elseif (option == setHairyChance)
		SetSliderDialogStartValue(OPubes.HairyChance)
		SetSliderDialogDefaultValue(25.0)
		SetSliderDialogRange(0.0, 100.0)
		SetSliderDialogInterval(1.0)
	endIf
EndEvent


Event OnOptionSliderAccept(int option, float value)
	if (option == setShavedChance)
		OPubes.ShavedChance = value as int
		SetSliderOptionValue(setShavedChance, OPubes.ShavedChance)

	elseif (option == setNormalChance)
		OPubes.NormalChance = value as int
		SetSliderOptionValue(setNormalChance, OPubes.NormalChance)

	elseif (option == setStylishChance)
		OPubes.StylishChance = value as int
		SetSliderOptionValue(setStylishChance, OPubes.StylishChance)

	elseif (option == setHairyChance)
		OPubes.HairyChance = value as int
		SetSliderOptionValue(setHairyChance, OPubes.HairyChance)
	endIf
EndEvent


event OnOptionKeyMapChange(int option, int keyCode, string conflictControl, string conflictName)
	If (option == setPubeCycleKey)
		bool continue = true

		if (conflictControl != "" && keyCode != 1)
			string msg

			if (conflictName != "")
				msg = "This key is already mapped to:\n\"" + conflictControl + "\"\n(" + conflictName + ")\n\nAre you sure you want to continue?"
			else
				msg = "This key is already mapped to:\n\"" + conflictControl + "\"\n\nAre you sure you want to continue?"
			endIf

			continue = ShowMessage(msg, true, "$opubes_message_box_option_yes", "$opubes_message_box_option_no")
		endIf

		if (continue)
			int previousKey = OPubes.PubesCycleKey
			OPubes.PubesCycleKey = keyCode
			SetKeymapOptionValue(setPubeCycleKey, keyCode)
			OPubes.UpdatePubesCycleKey(previousKey)
		endIf
	EndIf
endEvent


Event OnOptionMenuOpen(int option)
	if (option == setOBodyManualChangeBehaviour)
		SetMenuDialogOptions(obodyManualChangeBehaviours)
		SetMenuDialogStartIndex(OPubes.OBodyManualChangeBehaviour)
		SetMenuDialogDefaultIndex(0)
	elseif (option == setPubeActor)
		SetMenuDialogOptions(pubeTypeList)
		SetMenuDialogStartIndex(0)
		SetMenuDialogDefaultIndex(0)
	endIf
EndEvent


Event OnOptionMenuAccept(int option, int index)
	if (option == setOBodyManualChangeBehaviour)
		OPubes.OBodyManualChangeBehaviour = index
		SetMenuOptionValue(setOBodyManualChangeBehaviour, obodyManualChangeBehaviours[index])
	elseif (option == setPubeActor && index > 0)
		SetMenuOptionValue(setPubeActor, pubeTypeList[index])

		Actor actorInCrosshair = Game.GetCurrentCrosshairRef() as Actor
		OPubes.UpdateActorPubes(actorInCrosshair, pubeTypeList[index])
	endIf
EndEvent


event OnOptionHighlight(int option)
	if (option == setOBodyManualChangeBehaviour)
		SetInfoText("$opubes_highlight_obody_manual_behaviour")
	elseif (option == setShavedChance)
		SetInfoText("$opubes_highlight_shaved_chance")
	elseif (option == setNormalChance)
		SetInfoText("$opubes_highlight_normal_chance")
	elseif (option == setStylishChance)
		SetInfoText("$opubes_highlight_stylish_chance")
	elseif (option == setHairyChance)
		SetInfoText("$opubes_highlight_hairy_chance")
	elseif (option == setPubeCycleKey)
		SetInfoText("$opubes_highlight_cycle_key")
	elseif (option == setPubeActor)
		SetInfoText("$opubes_highlight_pube_actor")
	endif
endEvent
