/*=============================================================================
	UnNames.h: Header file registering global hardcoded Unreal names.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Macros.
-----------------------------------------------------------------------------*/

// Remove windows def.
#ifdef GetObject
#undef GetObject
#endif

// Define a message as an enumeration.
#ifndef REGISTER_NAME
	#define REGISTER_NAME(num,name) NAME_##name = num,
	#define REG_NAME_HIGH(num,name) NAME_##name = num,
	#define REGISTERING_ENUM
	enum EName {
#endif

/*-----------------------------------------------------------------------------
	Hardcoded names which are not messages.
-----------------------------------------------------------------------------*/

// Special zero value, meaning no name.
REG_NAME_HIGH(   0, None             )

// Class property types; these map straight onto hardcoded property types.
REGISTER_NAME(   1, ByteProperty     )
REGISTER_NAME(   2, IntProperty      )
REGISTER_NAME(   3, BoolProperty     )
REGISTER_NAME(   4, FloatProperty    )
REGISTER_NAME(   5, ObjectProperty   )
REGISTER_NAME(   6, NameProperty     )
REGISTER_NAME(   7, StringProperty   )
REGISTER_NAME(   8, ClassProperty    )
REGISTER_NAME(   9, ArrayProperty    )
REGISTER_NAME(  10, StructProperty   )
REGISTER_NAME(  11, VectorProperty   )
REGISTER_NAME(  12, RotatorProperty  )
REGISTER_NAME(  13, StrProperty      )//!!
REGISTER_NAME(  14, MapProperty      )//!!
REGISTER_NAME(  15, DelegateProperty )//!!
REGISTER_NAME(  16, InterfaceProperty)//!!

// Packages.
REGISTER_NAME(  20, Core			 )
REGISTER_NAME(  21, Engine			 )
REGISTER_NAME(  22, Editor           )
REGISTER_NAME(  23, UnrealI          )
REGISTER_NAME(  24, UnrealShare      )
REGISTER_NAME(  25, Fire		     )

// UnrealScript types.
REG_NAME_HIGH(  80, Byte		     )
REG_NAME_HIGH(  81, Int			     )
REG_NAME_HIGH(  82, Bool		     )
REG_NAME_HIGH(  83, Float		     )
REG_NAME_HIGH(  84, Name		     )
REG_NAME_HIGH(  85, String		     )
REG_NAME_HIGH(  86, Struct			 )
REG_NAME_HIGH(  87, Vector		     )
REG_NAME_HIGH(  88, Rotator	         )
REG_NAME_HIGH(  90, Color            )
REG_NAME_HIGH(  91, Plane            )
REG_NAME_HIGH(  92, Pointer			 )
REG_NAME_HIGH(  93, Coords			 )
REG_NAME_HIGH(  94, Any				 )

// Keywords.
REGISTER_NAME( 100, Begin			 )
REG_NAME_HIGH( 102, State            )
REG_NAME_HIGH( 103, Function         )
REG_NAME_HIGH( 104, Self             )
REG_NAME_HIGH( 105, True             )
REG_NAME_HIGH( 106, False            )
REG_NAME_HIGH( 107, Transient        )
REG_NAME_HIGH( 108, TypeCast         )
REG_NAME_HIGH( 109, Construct        )
REG_NAME_HIGH( 117, Enum			 )
REG_NAME_HIGH( 119, Replication      )
REG_NAME_HIGH( 120, Reliable         )
REG_NAME_HIGH( 121, Unreliable       )
REG_NAME_HIGH( 122, Always           )
REGISTER_NAME( 123, IfCondition		 )
REGISTER_NAME( 124, ElseIf			 )
REGISTER_NAME( 125, EndIf			 )
REGISTER_NAME( 126, Include			 )
REGISTER_NAME( 127, Define			 )
REGISTER_NAME( 128, IsDefined		 )
REGISTER_NAME( 129, NotDefined		 )
REGISTER_NAME( 130, Undefine		 )
REGISTER_NAME( 131, Counter			 )
REGISTER_NAME( 132, GetCounter		 )
REGISTER_NAME( 133, SetCounter		 )
REGISTER_NAME( 134, EngineVersion	 )

// Object class names.
REGISTER_NAME( 150, Field            )
REGISTER_NAME( 151, Object           )
REGISTER_NAME( 152, TextBuffer       )
REGISTER_NAME( 153, Linker           )
REGISTER_NAME( 154, LinkerLoad       )
REGISTER_NAME( 155, LinkerSave       )
REGISTER_NAME( 156, Subsystem        )
REGISTER_NAME( 157, Factory          )
REGISTER_NAME( 158, TextBufferFactory)
REGISTER_NAME( 159, Exporter         )
REGISTER_NAME( 160, StackNode        )
REGISTER_NAME( 161, Property         )
REGISTER_NAME( 162, Camera           )
REGISTER_NAME( 163, FontCharacter	 )

// Constants.
REG_NAME_HIGH( 600, Vect)
REG_NAME_HIGH( 601, Rot)
REG_NAME_HIGH( 602, RotAng)
REG_NAME_HIGH( 605, ArrayCount)
REG_NAME_HIGH( 606, EnumCount)
REG_NAME_HIGH( 607, Caller)
REG_NAME_HIGH( 608, SizeOf)
REG_NAME_HIGH( 609, CppText)
REG_NAME_HIGH( 610, NameOf)

// Flow control.
REG_NAME_HIGH( 620, Else)
REG_NAME_HIGH( 621, If)
REG_NAME_HIGH( 622, Goto)
REG_NAME_HIGH( 623, Stop)
REG_NAME_HIGH( 625, Until)
REG_NAME_HIGH( 626, While)
REG_NAME_HIGH( 627, Do)
REG_NAME_HIGH( 628, Break)
REG_NAME_HIGH( 629, For)
REG_NAME_HIGH( 630, ForEach)
REG_NAME_HIGH( 631, Assert)
REG_NAME_HIGH( 632, Switch)
REG_NAME_HIGH( 633, Case)
REG_NAME_HIGH( 634, Default)
REG_NAME_HIGH( 635, Continue)

// Variable overrides.
REG_NAME_HIGH( 640, Private)
REG_NAME_HIGH( 641, Const)
REG_NAME_HIGH( 642, Out)
REG_NAME_HIGH( 643, Export)
REG_NAME_HIGH( 646, Skip)
REG_NAME_HIGH( 647, Coerce)
REG_NAME_HIGH( 648, Optional)
REG_NAME_HIGH( 649, Input)
REG_NAME_HIGH( 650, Config)
REG_NAME_HIGH( 652, Immutable)
REG_NAME_HIGH( 653, EditConst)
REG_NAME_HIGH( 654, Localized)
REG_NAME_HIGH( 655, GlobalConfig)
REG_NAME_HIGH( 656, SafeReplace)
REG_NAME_HIGH( 657, New)
REG_NAME_HIGH( 658, Atomic)
REG_NAME_HIGH( 659, Protected)

// Class overrides.
REG_NAME_HIGH( 660, Expands)
REG_NAME_HIGH( 661, Intrinsic)
REG_NAME_HIGH( 662, Within)
REG_NAME_HIGH( 663, Abstract)
REG_NAME_HIGH( 664, Package)
REG_NAME_HIGH( 665, Guid)
REG_NAME_HIGH( 666, Parent)
REG_NAME_HIGH( 667, Class)
REG_NAME_HIGH( 668, Extends)
REG_NAME_HIGH( 669, NoExport)
REG_NAME_HIGH( 670, NoUserCreate)
REG_NAME_HIGH( 671, PerObjectConfig)
REG_NAME_HIGH( 672, NativeReplication)
REG_NAME_HIGH( 673, DependsOn)
REG_NAME_HIGH( 674, RuntimeStatic)

// State overrides.
REG_NAME_HIGH( 675, Auto)
REG_NAME_HIGH( 676, Ignores)

// Calling overrides.
REG_NAME_HIGH( 680, Global)
REG_NAME_HIGH( 681, Super)
REG_NAME_HIGH( 682, Outer)

// Function overrides.
REG_NAME_HIGH( 690, Operator)
REG_NAME_HIGH( 691, PreOperator)
REG_NAME_HIGH( 692, PostOperator)
REG_NAME_HIGH( 693, Final)
REG_NAME_HIGH( 694, Iterator)
REG_NAME_HIGH( 695, Latent)
REG_NAME_HIGH( 696, Return)
REG_NAME_HIGH( 697, Singular)
REG_NAME_HIGH( 698, Simulated)
REG_NAME_HIGH( 699, Exec)
REG_NAME_HIGH( 700, Event)
REG_NAME_HIGH( 701, Static)
REG_NAME_HIGH( 702, Native)
REG_NAME_HIGH( 703, Invariant)
REG_NAME_HIGH( 704, Delegate)
REG_NAME_HIGH( 705, Virtual)
REG_NAME_HIGH( 706, Public)
REG_NAME_HIGH( 707, NoExportHeader)
REG_NAME_HIGH( 708, Server)
REG_NAME_HIGH( 709, Client)

// Variable declaration.
REG_NAME_HIGH( 710, Var)
REG_NAME_HIGH( 711, Local)
REG_NAME_HIGH( 712, Import)
REG_NAME_HIGH( 713, From)
REG_NAME_HIGH( 714, ImmutableWhenCooked)
REG_NAME_HIGH( 715, StrictConfig)
REG_NAME_HIGH( 716, ProtectedWrite)
REG_NAME_HIGH( 717, PrivateWrite)
REG_NAME_HIGH( 718, EditHide)
REG_NAME_HIGH( 719, EditTextBox)

// Special commands.
REG_NAME_HIGH( 720, Spawn)
REG_NAME_HIGH( 721, Array)
REG_NAME_HIGH( 722, Map)
REGISTER_NAME( 723, FindObject)
REG_NAME_HIGH( 724, SetHook)
REG_NAME_HIGH( 725, Remove)
REG_NAME_HIGH( 726, Has)
REG_NAME_HIGH( 727, Empty)
REGISTER_NAME( 728, GetDefaultObject)
REG_NAME_HIGH( 729, Length)
REG_NAME_HIGH( 730, RemoveItem)
REG_NAME_HIGH( 731, Insert)
REG_NAME_HIGH( 732, Find)
REG_NAME_HIGH( 733, Add)
REG_NAME_HIGH( 734, AddItem)
REG_NAME_HIGH( 735, RemoveValue)
REG_NAME_HIGH( 736, Sort)
REG_NAME_HIGH( 737, InsertItem)

// Misc.
REGISTER_NAME( 740, Tag)
REGISTER_NAME( 741, Location)
REGISTER_NAME( 742, Role)
REGISTER_NAME( 743, RemoteRole)
REGISTER_NAME( 744, System)
REGISTER_NAME( 745, User)
REGISTER_NAME( 746, DuplicateTransient)
REGISTER_NAME( 747, NoImport)
REGISTER_NAME(748, EditInlineUse)
REGISTER_NAME(749, NoClear)
REGISTER_NAME(750, RepNotify)
REGISTER_NAME(751, RepRetry)
REGISTER_NAME(752, Interp)
REGISTER_NAME(753, NonTransactional)
REGISTER_NAME(754, Deprecated)
REGISTER_NAME(755, Instanced)
REGISTER_NAME(756, DataBinding)
REGISTER_NAME(757, EditorOnly)
REGISTER_NAME(758, NotForConsole)
REGISTER_NAME(759, Archetype)

// Log messages.
REGISTER_NAME( 760, Log)
REGISTER_NAME( 761, Critical)
REGISTER_NAME( 762, Init)
REGISTER_NAME( 763, Exit)
REGISTER_NAME( 764, Cmd)
REGISTER_NAME( 765, Play)
REGISTER_NAME( 766, Console)
REGISTER_NAME( 767, Warning)
REGISTER_NAME( 768, ExecWarning)
REGISTER_NAME( 769, ScriptWarning)
REGISTER_NAME( 770, ScriptLog)
REGISTER_NAME( 771, Dev)
REGISTER_NAME( 772, DevNet)
REGISTER_NAME( 773, DevPath)
REGISTER_NAME( 774, DevNetTraffic)
REGISTER_NAME( 775, DevAudio)
REGISTER_NAME( 776, DevLoad)
REGISTER_NAME( 777, DevSave)
REGISTER_NAME( 778, DevGarbage)
REGISTER_NAME( 779, DevKill)
REGISTER_NAME( 780, DevReplace)
REGISTER_NAME( 781, DevMusic)
REGISTER_NAME( 782, DevSound)
REGISTER_NAME( 783, DevCompile)
REGISTER_NAME( 784, DevBind)
REGISTER_NAME( 785, Localization)
REGISTER_NAME( 786, Compatibility)
REGISTER_NAME( 787, NetComeGo)
REGISTER_NAME( 788, Title)
REGISTER_NAME( 789, Error)
REGISTER_NAME( 790, Heading)
REGISTER_NAME( 791, SubHeading)
REGISTER_NAME( 792, FriendlyError)
REGISTER_NAME( 793, Progress)
REGISTER_NAME( 794, UserPrompt)
REGISTER_NAME( 795, ServerLog)
// Shambler
REGISTER_NAME( 796, RPCFlood)
#ifdef UTPG_MD5
REGISTER_NAME( 797, DevMD5)
#endif
REGISTER_NAME( 798, DevPhysics)
REGISTER_NAME( 799, DevGraphics)

// Console text colors.
REGISTER_NAME( 800, White)
REGISTER_NAME( 801, Black)
REGISTER_NAME( 802, Red)
REGISTER_NAME( 803, Green)
REGISTER_NAME( 804, Blue)
REGISTER_NAME( 805, Cyan)
REGISTER_NAME( 806, Magenta)
REGISTER_NAME( 807, Yellow)
REGISTER_NAME( 808, DefaultColor)

// Misc.
REGISTER_NAME( 820, KeyType)
REGISTER_NAME( 821, KeyEvent)
REGISTER_NAME( 822, Write)
REGISTER_NAME( 823, Message)
REGISTER_NAME( 824, InitialState)
REGISTER_NAME( 825, Texture)
REGISTER_NAME( 826, Sound)
REGISTER_NAME( 827, FireTexture)
REGISTER_NAME( 828, IceTexture)
REGISTER_NAME( 829, WaterTexture)
REGISTER_NAME( 830, WaveTexture)
REGISTER_NAME( 831, WetTexture)
REGISTER_NAME( 832, Main)
REGISTER_NAME( 833, NotifyLevelChange)
REGISTER_NAME( 834, VideoChange)
REGISTER_NAME( 835, SendText)
REGISTER_NAME( 836, SendBinary)
REGISTER_NAME( 837, ConnectFailure)
REGISTER_NAME( 838, OnPropertyChange)
REG_NAME_HIGH( 839, Button)
REGISTER_NAME( 840, StaticPropertyChange)
REG_NAME_HIGH( 910, Template)
REGISTER_NAME( 911, UTrace)
REG_NAME_HIGH( 912, EditInline)
REG_NAME_HIGH( 913, AutoDestruct)
REGISTER_NAME(914, SerializeText)
REGISTER_NAME(915, CrossLevel)
REGISTER_NAME(916, CrossLevelActive)
REGISTER_NAME(917, CrossLevelPassive)
REG_NAME_HIGH(918, Interface)
REG_NAME_HIGH(919, NativeOnly)
REG_NAME_HIGH(920, Inherits)
REG_NAME_HIGH(921, EditInlineNew)
REG_NAME_HIGH(922, NotEditInlineNew)
REG_NAME_HIGH(923, Placeable)
REG_NAME_HIGH(924, NotPlaceable)
REG_NAME_HIGH(925, HideDropDown)
REG_NAME_HIGH(926, PerObjectLocalized)
REG_NAME_HIGH(927, NonTransient)
REG_NAME_HIGH(928, ShowCategories)
REG_NAME_HIGH(929, ForceScriptOrder)
REG_NAME_HIGH(930, HideCategories)
REG_NAME_HIGH(931, ClassGroup)
REG_NAME_HIGH(932, AutoExpandCategories)
REG_NAME_HIGH(933, AutoCollapseCategories)
REG_NAME_HIGH(934, DontAutoCollapseCategories)
REG_NAME_HIGH(935, DontSortCategories)
REG_NAME_HIGH(936, CollapseCategories)
REG_NAME_HIGH(937, DontCollapseCategories)
REG_NAME_HIGH(938, Implements)
REG_NAME_HIGH(939, K2Call)
REG_NAME_HIGH(940, K2Override)
REG_NAME_HIGH(941, K2Pure)
REG_NAME_HIGH(942, Components)
REG_NAME_HIGH(943, Actor)
REG_NAME_HIGH(944, EditFixedSize)
REG_NAME_HIGH(945, AkEvent)
REG_NAME_HIGH(946, Stripped)
REG_NAME_HIGH(947, Context)

// Invalid entry.
REGISTER_NAME( 999, Invalid)

/*-----------------------------------------------------------------------------
	Special engine-generated probe messages.
-----------------------------------------------------------------------------*/

//
//warning: All probe entries must be filled in, otherwise non-probe names might be mapped
// to probe name indices.
//
#define NAME_PROBEMIN ((EName)300)
#define NAME_PROBEMAX ((EName)364)

// Creation and destruction.
REGISTER_NAME( 300, Spawned			 ) // Sent to actor immediately after spawning.
REGISTER_NAME( 301, Destroyed        ) // Called immediately before actor is removed from actor list.

// Gaining/losing actors.
REGISTER_NAME( 302, GainedChild		 ) // Sent to a parent actor when another actor attaches to it.
REGISTER_NAME( 303, LostChild		 ) // Sent to a parent actor when another actor detaches from it.
REGISTER_NAME( 304, OwnerChanged	 ) // Sent a message to child actor when owner was changed.
REGISTER_NAME( 305, Probe5			 )

// Triggers.
REGISTER_NAME( 306, Trigger			 ) // Message sent by Trigger actors.
REGISTER_NAME( 307, UnTrigger		 ) // Message sent by Trigger actors.

// Physics & world interaction.
REGISTER_NAME( 308, Timer			 ) // The per-actor timer has fired.
REGISTER_NAME( 309, HitWall			 ) // Ran into a wall.
REGISTER_NAME( 310, Falling			 ) // Actor is falling.
REGISTER_NAME( 311, Landed			 ) // Actor has landed.
REGISTER_NAME( 312, ZoneChange		 ) // Actor has changed into a new zone.
REGISTER_NAME( 313, Touch			 ) // Actor was just touched by another actor.
REGISTER_NAME( 314, UnTouch			 ) // Actor touch just ended, always sent sometime after Touch.
REGISTER_NAME( 315, Bump			 ) // Actor was just touched and blocked. No interpenetration. No UnBump.
REGISTER_NAME( 316, BeginState		 ) // Just entered a new state.
REGISTER_NAME( 317, EndState		 ) // About to leave the current state.
REGISTER_NAME( 318, BaseChange		 ) // Sent to actor when its floor changes.
REGISTER_NAME( 319, Attach			 ) // Sent to actor when it's stepped on by another actor.
REGISTER_NAME( 320, Detach			 ) // Sent to actor when another actor steps off it.
REGISTER_NAME( 321, ActorEntered	 ) // Sent to a ZoneInfo actor when an actor enters.
REGISTER_NAME( 322, ActorLeaving	 ) // Sent to a ZoneInfo actor when an actor is leaving.
REGISTER_NAME( 323, KillCredit		 ) // Actor has just received credit for a kill.
REGISTER_NAME( 324, AnimEnd			 ) // Animation sequence has ended.
REGISTER_NAME( 325, EndedRotation	 ) // Physics based rotation just ended.
REGISTER_NAME( 326, InterpolateEnd   ) // Movement interpolation sequence finished.
REGISTER_NAME( 327, EncroachingOn    ) // Encroaching on another actor.
REGISTER_NAME( 328, EncroachedBy     ) // Being encroached by another actor.
REGISTER_NAME( 329, FootZoneChange   ) // Pawn's feet changed zones
REGISTER_NAME( 330, HeadZoneChange   ) // Pawn's head changed zones
REGISTER_NAME( 331, PainTimer        ) // pain timer expired
REGISTER_NAME( 332, SpeechTimer      ) // speech timer expired
REGISTER_NAME( 333, MayFall 		 )
REGISTER_NAME( 334, Probe34			 )

// Kills.
REGISTER_NAME( 335, Die				 ) // Actor died (sent if specific die message not processed).

// Updates.
REGISTER_NAME( 336, Tick			 ) // Clock tick update for nonplayer.
REGISTER_NAME( 337, PlayerTick		 ) // Clock tick update for player.
REGISTER_NAME( 338, Expired		     ) // Actor's LifeSpan expired.
REGISTER_NAME( 339, Probe39			 )

// AI.
REGISTER_NAME( 340, SeePlayer        ) // Can see player.
REGISTER_NAME( 341, EnemyNotVisible  ) // Current Enemy is not visible.
REGISTER_NAME( 342, HearNoise        ) // Noise nearby.
REGISTER_NAME( 343, UpdateEyeHeight  ) // Update eye level (after physics).
REGISTER_NAME( 344, SeeMonster       ) // Can see non-player.
REGISTER_NAME( 345, SeeFriend        ) // Can see non-player friend.
REGISTER_NAME( 346, SpecialHandling	 ) // Navigation point requests special handling.
REGISTER_NAME( 347, BotDesireability ) // Value of this inventory to bot.
REGISTER_NAME( 348, Probe48			 )
REGISTER_NAME( 349, Probe49			 )
REGISTER_NAME( 350, Probe50			 )
REGISTER_NAME( 351, Probe51			 )
REGISTER_NAME( 352, Probe52			 )
REGISTER_NAME( 353, Probe53			 )
REGISTER_NAME( 354, Probe54			 )
REGISTER_NAME( 355, Probe55			 )
REGISTER_NAME( 356, Probe56			 )
REGISTER_NAME( 357, Probe57			 )
REGISTER_NAME( 358, Probe58			 )
REGISTER_NAME( 359, Probe59			 )
REGISTER_NAME( 360, Probe60			 )
REGISTER_NAME( 361, Probe61			 )

#ifdef UTPG_PACKAGE_LOADED
REGISTER_NAME( 362, PackageLoaded)
#else
REGISTER_NAME( 362, Probe62			 )
#endif

// Special tag meaning 'All probes'.
REGISTER_NAME( 363, All				 ) // Special meaning, not a message.


/*-----------------------------------------------------------------------------
	Closing.
-----------------------------------------------------------------------------*/

#ifdef REGISTERING_ENUM
	};
	#undef REGISTER_NAME
	#undef REG_NAME_HIGH
	#undef REGISTERING_ENUM
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
