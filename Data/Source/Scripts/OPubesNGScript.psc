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

Int property SlotToUse auto

bool property EnableMales auto


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
	; earlier versions registered for OBody's mod event
	UnregisterForAllModEvents()
EndFunction


Event OnKeyDown(int KeyPress)
	if Utility.IsInMenuMode() || UI.IsTextInputEnabled()
		return
	endif

	if KeyPress == PubesCycleKey && KeyPress != 1
		Actor actorInCrosshair = Game.GetCurrentCrosshairRef() as Actor

		if actorInCrossHair == none
			actorInCrossHair = PlayerRef
		endif

		OPubesNative.ChangeActorPubes(actorInCrosshair)
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


Function UpdateActorPubes(Actor Act, String PubesType)
	OPubesNative.SetActorPubesType(Act, PubesType)
EndFunction


Bool Function IsFemale(Actor Act)
	return Act.GetLeveledActorBase().GetSex() == 1
EndFunction
