# Cyberpunk Operative
## Game Design Document — v0.1

**Working title:** TBD  
**Engine:** Unreal Engine 5  
**Platform:** PC initially  
**Mode:** Single-player  
**Genre:** Open-world cyberpunk flight / high-speed stealth-action  
**Campaign target:** ~6–8 hours per campaign  
**Visual target:** Stylised neon graphic novel / comic treatment  
**Production approach:** Store-asset-first, tightly scoped around the core fantasy

---

# 1. High Concept

The player is a high-tech field operative employed by a megacorporation in a vertically built cyberpunk city.

They are not a rebel fighting corporate power. They are one of its privileged instruments.

The player flies a highly capable VTOL craft freely through a contested city, receives corporate assignments, scans opportunities, and inserts directly into compact stealth arenas. Ground missions are built around extremely fast cycles of observation, movement and violence.

The operative possesses a perception-enhancement ability that can slow their subjective experience of time for roughly five seconds. While active, the player primarily observes, scans, hacks, marks targets and chooses actions. The ability then requires approximately ten seconds to recharge.

This does **not** create a rigid five-second planning / ten-second execution turn structure. It is an ability that naturally encourages a cadence of:

**observe → choose → explode into motion → disappear → reassess**

The operative is superhuman because of **speed, stealth, information and positioning**, not durability or complex combat mastery.

---

# 2. Player Fantasy

> Be the megacorporation's terrifyingly efficient field operative: arrive from the sky, understand a hostile site in seconds, tear through its security before anyone can react, disappear into the city, and know that normal law applies to you only when somebody more powerful insists that it does.

The desired player experience combines:

- freedom and speed while flying through the city;
- the confidence of superior corporate technology;
- short periods of intense tactical observation;
- fluid contextual parkour;
- decisive stealth kills;
- rapid recovery from partial detection;
- escalating pressure if the player spends too long on-site;
- political consequences rather than conventional morality systems;
- a campaign that continues through both success and failure.

---

# 3. Design Pillars

## 3.1 Speed Is Power

The operative wins by moving faster than security can understand what is happening.

Standing still and exchanging gunfire is not the intended power fantasy.

Movement, surprise, cyberware and information should consistently outperform attritional combat.

**Test:** if optimal play regularly consists of remaining behind cover and trading shots, the design has drifted.

## 3.2 Read, Commit, Vanish

The player gets short opportunities to understand the tactical space, followed by rapid action.

The perception ability exists to let the player make sophisticated decisions without requiring extreme twitch skill.

A successful sequence should often look impossible at full speed:

1. observe;
2. identify threats and route;
3. trigger hacks;
4. sprint/vault/climb;
5. eliminate or bypass a guard;
6. break contact;
7. regain concealment.

## 3.3 Seen Is Not Failed

Stealth is about preventing enemies from maintaining useful information about the player.

A momentary sighting should create a problem rather than invalidate the mission.

Detection progresses through understandable stages.

> **Being seen is recoverable. Being tracked is dangerous.**

## 3.4 The Aircraft and Operative Are One System

Flight is not a disconnected travel minigame.

The aircraft:
- transports the player;
- scans territory;
- discovers opportunities;
- provides pre-mission intelligence;
- cloaks above mission sites;
- creates the mission's primary escalation clock;
- provides remote support;
- inserts the player;
- extracts them;
- can become overt fire support after decloaking.

Aircraft progression must therefore affect both flight and ground missions.

## 3.5 Corporate Power, Not Street Crime

The player's relationship with the police is intentionally unlike a conventional wanted system.

The player is authorised to commit acts that would be criminal for ordinary citizens.

Police response depends on jurisdiction, political protection, collateral damage, evidence, local influence and whether the player has exceeded the corporation's authority.

The player is above ordinary law, but not above competing power.

## 3.6 Procedural Variety, Authored Quality

Mission replayability comes from recombining known-good tactical ingredients inside deliberately constructed stealth spaces.

The game should **not** attempt to generate arbitrary parkour levels.

Procedural systems configure:
- security;
- objectives;
- routes;
- access;
- extraction;
- faction doctrine;
- complications;
- rival intervention;
- world context.

Level geometry remains primarily authored.

---

# 4. Explicit Non-Goals

The game is not:
- a multiplayer title;
- a realistic flight simulator;
- a large-scale pedestrian open-world simulation;
- a Hitman-style social-stealth game;
- a deep shooter;
- a melee combat game;
- a loot shooter;
- an immersive sim requiring dozens of systemic interactions per room;
- a city-life simulator;
- a fully simulated faction grand-strategy game;
- a procedurally generated 3D level system;
- a dialogue-heavy RPG;
- a photorealistic graphical showcase.

Civilians primarily provide visual population and atmosphere rather than tactical stealth gameplay.

Ground exploration outside missions is not a core feature.

---

# 5. Core Game Loop

```text
FLY THROUGH CITY
      ↓
Receive corporate assignment
or discover optional opportunity
      ↓
Assess territory / police / faction conditions
      ↓
Approach target site
      ↓
Aircraft scan + variable intelligence
      ↓
Choose authored insertion point
      ↓
Cloak aircraft above target
      ↓
Launch operative into mission
      ↓
Observe / infiltrate / execute objective
      ↓
Rival trace escalates over time
      ↓
Choose extraction point
      ↓
Covert pickup
OR
hot extraction with aircraft fire support
      ↓
Return to flight
      ↓
World / campaign state changes
      ↓
New contracts become valid
```

The player should spend very little time in non-interactive transitions between these states.

---

# 6. Open World

## 6.1 City Structure

The game uses one compact, vertically layered cyberpunk city rather than a geographically enormous world.

Target full-game structure:
- approximately 6–8 major districts;
- strong vertical separation;
- towers;
- elevated transport infrastructure;
- rooftop spaces;
- industrial levels;
- corporate enclaves;
- dense air routes;
- restricted zones.

The city should feel large because it has **height, speed and territorial variation**, not because the player must cross kilometres of empty terrain.

---

# 7. Flight

## 7.1 Aircraft Handling

The aircraft uses an **arcade VTOL hybrid** model.

At low speed:
- precise hover;
- strong vertical movement;
- lateral movement;
- controlled rotation;
- strong braking;
- easy positioning over mission sites.

At high speed:
- more forward momentum;
- more substantial turning arcs;
- stronger sensation of weight;
- city-crossing becomes satisfying traversal rather than free-camera movement.

Physics should support the fantasy rather than simulate aerodynamics.

## 7.2 Camera

Aircraft:
- cockpit view;
- third-person chase view.

Ground:
- fixed third-person.

No first-person ground mode is required.

## 7.3 Open-World Flight Activity

Flight supports:
- travelling between assignments;
- scanning locations;
- discovering optional contracts;
- reading territory;
- avoiding hostile airspace;
- police scans;
- corporate interception;
- faction patrols;
- pursuit;
- jamming;
- countermeasure use.

Dogfighting is a valid expansion direction but **not required for the first playable**.

The first version of hostile airspace should be solvable through:
- speed;
- altitude;
- urban terrain;
- route choice;
- stealth;
- countermeasures.

---

# 8. Territory and Factions

## 8.1 District Control

Districts are controlled primarily by:
- gangs;
- police/security authority.

Gangs are effectively proxies for different corporations.

Their equipment, resources and strategic interests reflect their sponsor.

Police function mechanically somewhat like another territorial faction, but are funded and directed by a **corporate council**.

The player's employer is one equal member of that council.

Therefore police are neither automatically friendly nor automatically hostile.

## 8.2 Territorial Change

Territory uses **controlled dynamism**.

The world begins in a designed state.

Control can change through:
- campaign operations;
- mission success;
- mission failure;
- explicit strategic decisions;
- limited systemic events.

The game does not simulate an unrestricted autonomous faction war.

Territory state may modify:
- available contracts;
- airspace threat;
- police tolerance;
- gang presence;
- mission defenders;
- intelligence quality;
- rival intervention;
- corporate influence.

---

# 9. Authority / Heat

Police response is governed by a legible combined Authority/Heat model.

Inputs may include:
- whether the current operation is officially sanctioned;
- attacks on police;
- collateral damage;
- protected locations;
- evidence;
- territorial ownership;
- corporate political influence;
- existing campaign state.

Possible escalation:

```text
TOLERATED
↓
OBSERVED
↓
QUESTIONED / SCANNED
↓
INTERCEPTED
↓
PURSUED
↓
ARMED RESPONSE
```

Police may therefore watch the player openly assassinate a sanctioned target without intervening, yet attack the same operative elsewhere for violating a council-protected site.

The system should communicate *why* the player's protection is changing.

---

# 10. Contract Discovery

Contracts come from two sources.

## Corporate Dispatch

The employer sends priority operations.

These drive:
- career progression;
- campaign structure;
- corporate politics;
- major narrative blocks.

## Discovered Opportunities

Flying, scanning, faction intelligence and territory state can reveal optional work.

These support:
- progression;
- local territorial effects;
- intelligence;
- side conflicts;
- alternate campaign prerequisites.

The aircraft is the player's main hub. There is no walkable corporate headquarters.

---

# 11. Mission Architecture

## 11.1 Mission Length

Most missions should take roughly **5–10 minutes**.

The game may also contain:
- very short micro-contracts;
- occasional 15–25 minute major operations.

Long missions should be exceptional rather than the production baseline.

## 11.2 Mission Archetypes

Use approximately 5–6 robust archetypes rather than arbitrary procedural objectives.

### Assassinate
Reach and eliminate a specific target.

### Acquire
Steal physical technology, data or evidence.

### Extract
Recover or abduct a person or object.

### Sabotage
Plant a device, malware package or explosive; alter or destroy infrastructure.

### Recover / Rescue
Reach something before another faction and remove it from the site.

### Intercept / Deny
Prevent another faction from completing an operation.

Objectives should usually resolve quickly once reached.

The gameplay challenge lies in reaching the objective, controlling the security situation and escaping.

Avoid prolonged interaction bars.

---

# 12. Procedural Mission Configuration

Mission sites are authored tactical arenas.

Each exposes controlled procedural sockets for:
- guard spawn sets;
- patrol routes;
- cameras;
- sensors;
- drones;
- doors;
- access restrictions;
- alarm systems;
- objectives;
- target routes;
- extraction zones;
- insertion zones;
- faction overlays;
- environmental complications;
- limited modular geometry.

Procedural generation must select only validated combinations.

The system should prefer:

> **authored possibility spaces, procedurally selected**

rather than:

> **procedurally generated level layouts**

This preserves the quality of parkour routing.

---

# 13. Mission-Space Integration

Use a hybrid approach.

Some mission content can exist directly inside the open world:
- rooftops;
- exterior compounds;
- small facilities;
- balconies;
- infrastructure platforms.

Larger interiors can use controlled streamed/instanced gameplay spaces associated with their visible city location.

The player should perceive physical continuity even when the underlying technical implementation separates the mission space.

---

# 14. Reconnaissance

While hovering near a mission site, the aircraft can perform reconnaissance.

Intel quality varies.

Potential information:
- approximate target location;
- defender faction;
- guard density;
- known cameras;
- known drones;
- access points;
- insertion sockets;
- extraction sockets;
- security systems;
- environmental hazards.

Intel completeness can depend on:
- aircraft sensor upgrades;
- corporate status;
- sponsor support;
- previous missions;
- territory control;
- acquired intelligence.

Poor intel should create improvisation rather than unavoidable failure.

---

# 15. Insertion

The player chooses from authored insertion sockets.

Examples:
- rooftop;
- balcony;
- service gantry;
- alley;
- industrial deck;
- maintenance platform.

Once chosen, insertion is automatic and fast.

The craft positions itself and launches the operative downward from its underside.

The insertion sequence should be a signature transition, but not a long cinematic.

---

# 16. Aircraft Cloak and Mission Pressure

Once positioned above the site, the aircraft cloaks.

The cloak is effectively the mission's primary pressure system.

There is no instant failure when the nominal cloak period ends.

Instead rival corporations progressively locate the craft.

## Escalation

### Stage 0 — Hidden
No confirmed external response.

### Stage 1 — Trace
Rival systems detect anomalies.

The player receives warnings that triangulation has begun.

### Stage 2 — Contact
Rival drones, scouts or operatives approach.

### Stage 3 — Intervention
Rival teams insert into the mission space.

Their objectives may conflict with both the player and local defenders.

### Stage 4 — Breach
Rival hackers establish access to the aircraft.

A visible final compromise process begins.

### Stage 5 — Compromise
The aircraft is hacked.

**Mission failure.**

This pressure should make slow missions increasingly complex rather than simply displaying a countdown.

---

# 17. Rival Operatives

Rival operatives are primarily **objective-driven agents**, not enemies spawned merely to attack the player.

They may:
- steal the objective;
- destroy it;
- extract a target;
- plant their own device;
- interfere with security;
- hunt the player when identified;
- prioritise hacking the aircraft;
- retreat after achieving their goal.

A late mission can therefore become a three-way tactical situation:

**player ↔ site defenders ↔ rival corporation**

Different corporations use distinct doctrines assembled from common systems.

Examples of doctrine variation:
- hunter-killers;
- fast objective racers;
- stealth specialists;
- drone-heavy units;
- cyberwarfare teams;
- heavily monitored security;
- mobile pursuit teams.

Avoid bespoke AI architectures for every corporation.

---

# 18. Perception Mode

## 18.1 Purpose

Perception mode is the signature ground ability.

It allows the player to process a complex tactical situation quickly without requiring exceptional reaction speed.

## 18.2 Initial Tuning Target

Approximately:
- **5 seconds maximum active use**
- **10 seconds recharge**

These are tuning targets, not immutable values.

## 18.3 Behaviour

Perception mode substantially slows subjective time and restricts ordinary free movement.

It is primarily used to:
- identify guards;
- inspect alert states;
- inspect sightlines;
- reveal valid parkour actions;
- select contextual targets;
- identify hackable devices;
- queue/select a hack;
- identify escape routes;
- select a takedown target;
- assess extraction direction.

It should not become five seconds of effortless bullet-time shooting.

The player can cancel early.

The ability recharges continuously after use.

There is **no forced alternating planning/execution turn structure**.

---

# 19. Movement

## 19.1 Design

Movement is contextual and forgiving rather than precision-platform based.

Core movement:
- walk;
- sprint;
- crouch;
- jump;
- vault;
- mantle;
- climb;
- slide;
- ledge interaction;
- contextual cover movement where useful.

Certain actions can receive stronger target assistance:
- takedown;
- specific vault;
- grapple/perch if implemented;
- selected hack target;
- contextual traversal point.

The game should favour flow over mechanically demanding traversal inputs.

## 19.2 Animation Scope

Initial animation design is constrained by the owned animation packs.

The existing Open World animation set covers vaulting, climbing, falling, jumping, crawling, sliding, cover movement, and armed locomotion.

The existing Fighting animation set provides a broad range of unarmed movement and combat animations, but no bespoke assassination/pounce system is assumed.

Therefore:
- no bespoke pounce is required initially;
- one contextual takedown is sufficient;
- new movement verbs should require either existing animation coverage, an inexpensive store asset, or a strong core-design justification.

---

# 20. Stealth

## 20.1 Detection States

Suggested behavioural ladder:

```text
UNAWARE
↓
SUSPICIOUS
↓
INVESTIGATING
↓
VISUAL CONTACT
↓
LOCAL PURSUIT
↓
CONFIRMED INTRUSION
↓
SITE ALERT
```

Individual enemies do not magically share exact player position.

Information should propagate through plausible systems:
- shouting;
- radios;
- alarms;
- cameras;
- network systems;
- direct communication.

Breaking line of sight and moving rapidly can prevent escalation.

## 20.2 Detection Fairness

Detection must prioritise readability over simulation accuracy.

Players should be able to tell:
- who has seen them;
- how certain that enemy is;
- whether information has propagated;
- who is searching;
- whether their last known position is still relevant.

Detection feedback must not depend on colour alone.

---

# 21. Bodies

Dead/incapacitated guards remain in the environment.

If discovered, a body creates local suspicion or alert escalation.

There is **no manual carrying or body-hiding system**.

This keeps consequences without slowing the game with corpse management.

---

# 22. Enemy Archetypes

Use roughly five behavioural archetypes.

## Guard
Patrol, observe, shoot, investigate, raise alarms.

## Watcher
Camera operator, drone operator or sensor-focused enemy with strong detection.

## Tracker
Better at maintaining pursuit and communicating the player's last known position.

## Controller
Manipulates the arena:
- locks doors;
- activates security;
- deploys drones;
- alters routes;
- escalates systems.

## Rival Operative
More capable agent using cyberware and goal-driven behaviour.

Factions modify these roles rather than replacing them.

---

# 23. Combat

Combat is intentionally compact.

The player is dangerous when:
- attacking first;
- moving;
- exploiting surprise;
- using cyberware;
- attacking from advantageous position.

The player is vulnerable when:
- stationary;
- surrounded;
- exchanging sustained fire.

## Weapons

Small roster.

Potential classes:
- pistol;
- machine pistol / SMG;
- compact rifle or marksman weapon;
- shotgun.

Meaningful properties may include:
- suppressed;
- loud;
- armour penetration;
- stun/non-lethal;
- compact;
- high stopping power.

No rarity tiers.

No loot colour system.

No extensive attachment simulator.

---

# 24. Takedowns

Initial version:

> one reliable, fast contextual stealth takedown

Requirements:
- target in valid range;
- valid relative orientation;
- safe animation alignment;
- short duration;
- strong feedback.

Do not build a large assassination animation library for the first playable.

---

# 25. Cyberware

The complete game should contain approximately **6–8 meaningful cyberware abilities**.

The player equips approximately **2–3 at once**.

Possible pool:
- short personal cloak;
- movement boost/dash;
- sensor enhancement;
- remote hack;
- decoy;
- EMP;
- jammer;
- specialised mobility ability.

Cyberware should create **new possibilities**, not predominantly percentage bonuses.

Perception mode is treated as a foundational operative ability rather than simply one arbitrary loadout slot.

---

# 26. Hacking

Hacking is contextual and directly supports stealth.

There is no hacking minigame.

During perception mode the player can identify nearby networked objects and choose effects allowed by equipped cyberware.

Examples:
- open/lock door;
- disable camera;
- loop camera;
- distract;
- trigger machinery;
- disable drone;
- generate false alarm;
- interfere with sensors;
- alter access state.

The available verbs depend on loadout.

The goal is to make hacking part of the **observe → manipulate → move** loop.

---

# 27. Aircraft Support Loadout

The aircraft equips approximately 2–3 support systems.

Potential support:
- enhanced scan;
- network intrusion;
- decoy signal;
- EMP;
- remote sensor;
- counter-hacking;
- jammer;
- supply deployment;
- weapon support.

Before detection, support should favour information and stealth.

After decloaking, the aircraft may use overt fire support.

Aircraft upgrades therefore alter ground strategy, not merely flight statistics.

---

# 28. Extraction

Every arena contains several authored extraction sockets.

The current mission state determines which remain viable.

Security, rival presence and police activity may close extraction routes.

## Covert Extraction

If the aircraft remains cloaked and the zone is secure:
- player reaches extraction socket;
- craft positions itself;
- pickup occurs almost immediately.

## Hot Extraction

If the operation has escalated:
- player calls extraction;
- aircraft decloaks;
- aircraft moves into the area;
- enemy response increases;
- aircraft provides fire support where equipped;
- player makes a short final dash to extraction.

This creates an organic climax without forcing every mission to end loudly.

---

# 29. Civilians

Civilians are primarily atmospheric.

They are not core tactical actors.

They may:
- populate public spaces;
- flee from violence;
- visually reinforce district identity.

The game does not require:
- complex schedules;
- social stealth;
- civilian suspicion simulation;
- witness-management mechanics.

Collateral damage can still be abstractly reflected through Authority/Heat where appropriate.

---

# 30. Progression

Use three compact progression tracks.

## Operative

Unlock:
- cyberware;
- loadout slots;
- hacking options;
- specialised gear.

## Aircraft

Unlock:
- sensors;
- cloak improvements;
- handling options;
- countermeasures;
- support systems;
- eventual optional combat capability.

## Corporate Status

Unlock:
- higher-level contracts;
- restricted intelligence;
- greater legal authority;
- access permissions;
- sponsor relationships;
- increased political protection.

Progression should primarily add **capability and choice**, not health/damage inflation.

---

# 31. Internal Corporate Politics

The player's employer has its own doctrine and preferred technology.

However, the corporation is internally divided.

Different executives, departments, sponsors, and political blocs can support the operative.

Sponsor alignment can alter:
- contracts;
- intelligence;
- equipment access;
- political protection;
- strategic decisions.

This should remain a lightweight campaign-state system rather than a dense relationship simulator.

---

# 32. Narrative Tone

The surface tone is a **stylish corporate techno-thriller**.

The underlying world uses **dry corporate satire**.

Humour should emerge from:
- legal euphemism;
- liability language;
- HR-style framing of assassinations;
- corporate jurisdiction;
- police authorisation;
- executive sponsorship;
- absurdly formal treatment of violence.

The operative's work remains dangerous and consequential.

The game should not become a broad comedy.

---

# 33. Protagonist

The protagonist is defined but restrained.

They have:
- a recognisable voice;
- professional competence;
- baseline personality;
- corporate history.

They do not require a dialogue-heavy personal drama.

Their responses may change according to campaign state, including:
- loyalty;
- cynicism;
- sponsor alignment;
- conspiracy involvement;
- corporate-war involvement.

---

# 34. Campaign Structure

The campaign is generated from authored narrative blocks rather than a fixed linear mission list.

Architecture is roughly HTN-like.

Higher-level campaign goals decompose into available operations.

Narrative blocks have:
- prerequisites;
- setup;
- mission template;
- relevant world-state inputs;
- success result;
- failure result;
- follow-up tags;
- state changes.

The system selects locally valid next blocks.

It does **not** attempt to generate an entire branching narrative in advance.

---

# 35. Campaign Arc

## Phase 1 — Career

Every campaign starts here.

The player performs increasingly important corporate operations.

This establishes:
- corporate hierarchy;
- major factions;
- districts;
- rival corporations;
- police council;
- player progression.

## Phase 2 — Divergence

The accumulated consequences of choices, contract selection, success, failure, and sponsor relationships begin steering the campaign.

Two major directions emerge.

### Conspiracy

The player becomes involved in:
- hidden agendas;
- compromised institutions;
- internal secrets;
- covert programmes;
- unexpected relationships.

### Corporate War

Conflict escalates between:
- megacorporations;
- gangs;
- operatives;
- police blocs;
- territorial interests.

These are not mutually exclusive hard branches.

One can become dominant while blocks from the other remain available.

---

# 36. Player Campaign Agency

Campaign direction uses a mixture of implicit and explicit choice.

Implicit inputs include:
- missions accepted;
- missions rejected;
- target survival;
- success;
- failure;
- territorial outcomes;
- information retained or surrendered.

Occasionally the player receives explicit high-level choices.

Examples:
- support one proxy;
- suppress information;
- expose information;
- escalate conflict;
- prioritise police cooperation;
- support an internal sponsor.

---

# 37. Authored vs Generated Narrative

Routine campaign blocks use:

> authored structure + generated context

Variable elements may include:
- location;
- faction;
- target;
- objective;
- complication;
- local political state.

Important characters, revelations, confrontations, and climaxes use substantially more authored content.

Procedural systems contextualise the story rather than attempt to write its strongest dramatic moments.

---

# 38. Acting Against the Corporation

For most of the campaign the player remains inside the corporate system.

Possible disobedience includes:
- hiding intelligence;
- sparing a target;
- exceeding orders;
- choosing one internal sponsor over another;
- accepting unofficial work.

These choices can have consequences without immediately turning the protagonist into an outlaw.

Late-game campaign states may unlock:
- betrayal;
- defection;
- independence.

Those are exceptional outcomes rather than the assumed cyberpunk arc.

---

# 39. Campaign Endings

Endgame blocks are selected from accumulated state.

Possible dominant end conditions include:
- career apex;
- conspiracy resolution;
- corporate-war resolution;
- mixed outcomes;
- corporate betrayal/defection where prerequisites exist.

The campaign therefore ends because the current world state makes particular climax blocks valid, not because every run reaches the same mission.

---

# 40. Mission Failure

Mission failure is canon.

There is no expectation that the player reloads and retries until successful.

Failure may cause:
- target escape;
- rival acquisition of objective;
- territory change;
- lost corporate influence;
- sponsor reaction;
- increased police attention;
- new recovery operation;
- removal of one narrative possibility;
- creation of another.

Campaign blocks must be designed to tolerate likely failures.

---

# 41. Death

Player death is interpreted fictionally as catastrophic injury followed by corporate recovery/reconstruction.

The same operative returns.

Consequences:
- current mission fails;
- time/world state advances;
- rivals may achieve their goal;
- local state changes persist.

Death is not permanent character deletion.

---

# 42. Save Philosophy

The intended experience is **ironman-lite**.

The world preserves mission outcomes.

The design should not encourage reload-scumming as the standard response to bad outcomes.

Exact save implementation remains a technical decision, but player-facing expectations should make persistent consequences clear.

---

# 43. New Game+

After completing a campaign, the player can begin a harder/remixed New Game+ campaign.

Retain most:
- operative progression;
- aircraft progression.

Campaign structure and territorial state are regenerated/reset as appropriate.

New Game+ provides the long-term power progression that would otherwise undermine the early-career fantasy of a fresh normal campaign.

---

# 44. Visual Direction

Baseline style:

**neon graphic novel**

Use:
- simplified material response;
- strong silhouettes;
- deliberate colour blocking;
- emissive signage;
- stylised atmospheric haze;
- reduced material complexity;
- restrained outlines/cel treatment.

Stronger literal comic effects are reserved for high-impact states:
- perception mode;
- takedown;
- mission insertion;
- detection spike;
- objective completion;
- aircraft launch;
- campaign transitions.

Potential effects:
- halftone;
- ink edge;
- impact frames;
- graphic overlays;
- posterisation;
- panel-like transitions.

Readability at flight speed is more important than maximal rendering detail.

---

# 45. Asset Strategy

Production should default to:
1. owned assets;
2. inexpensive store assets;
3. procedural dressing;
4. custom content only where it creates the game's identity.

Custom effort should concentrate on:
- aircraft;
- protagonist readability;
- cyberware VFX;
- perception mode presentation;
- insertion/extraction;
- HUD;
- faction branding;
- signature mission props.

Do not custom-build generic furniture, pipes, doors, crowds or ordinary building shells unless necessary.

---

# 46. Audio Direction

Audio must support tactical understanding at speed.

Priority cues:
- partial detection;
- confirmed detection;
- propagation of alarm;
- rival trace stage;
- aircraft cloak state;
- cyberware ready;
- cyberware unavailable;
- police authority escalation;
- valid contextual takedown;
- extraction availability.

Each important state needs a recognisable audio cue without requiring the player to read the HUD.

Music can dynamically increase intensity as rival tracing progresses.

Hot extraction should produce an obvious musical and sonic payoff.

---

# 47. UI

## Flight HUD

Show only high-value information:
- heading/navigation;
- district/faction state;
- current Authority/Heat;
- aircraft cloak/support status;
- mission opportunity markers;
- scan feedback;
- hostile interception.

## Mission HUD

Primary information:
- current objective;
- perception availability;
- cyberware state;
- detection direction/state;
- aircraft cloak/trace state;
- extraction options;
- imminent rival intervention.

Avoid covering the screen with enemy markers during ordinary play.

Perception mode can temporarily increase information density.

---

# 48. Research Reference Games

## G-Rebels

Study:
- open-world aircraft traversal;
- cockpit fantasy;
- police pursuit;
- stealth aircraft upgrades;
- mission discovery.

Do not inherit:
- huge map scope;
- large profession/activity catalogue;
- flight-sim complexity.

## Assassin's Creed Mirage

Study:
- contextual parkour;
- traversal highways;
- reliable assassination;
- readable stealth;
- granular detection/search behaviour.

## Dishonored 2

Study:
- compact mission spaces;
- multiple traversal routes;
- vertical stealth;
- tools that create alternate approaches.

Avoid expanding into Dishonored's much wider systemic interaction set.

## Heat Signature

Study:
- separation between travel and infiltration;
- fast contract structure;
- assassination/theft/rescue objectives;
- planning time that allows complex execution;
- recovering from terrible situations through improvisation.

## Watch Dogs: Legion

Study:
- contextual environmental hacking;
- stealth cloak;
- city surveillance fiction;
- using a small number of hacking verbs during infiltration.

Do not inherit its population simulation or playable-NPC system.

---

# 49. Unreal Engine 5 Architecture

## 49.1 World Partition

Use World Partition for the main city.

Appropriate for:
- city streaming;
- flight-speed traversal;
- district partitioning;
- HLOD management.

Flight speed makes streaming stress-testing an early priority.

## 49.2 Data Layers

Use runtime Data Layers for controlled world-state changes such as:
- territorial overlays;
- changed checkpoints;
- damaged locations;
- alternate gang occupation;
- campaign-dependent dressing;
- mission setup variations.

Do not use them as a substitute for all dynamic gameplay state.

## 49.3 Level Instances

Use Level Instances/Packed Level Blueprints for:
- repeatable building shells;
- rooftop assemblies;
- mission structures;
- modular visual city components.

## 49.4 PCG

Use UE PCG primarily for:
- environmental dressing;
- prop distribution;
- façade variation;
- clutter;
- controlled mission socket population.

Do **not** make PCG responsible for proving whether a stealth encounter is navigable or enjoyable.

## 49.5 AI

### Mission AI
Use conventional Actors/Characters plus StateTree and navigation.

StateTree is a good conceptual match for:
- unaware;
- suspicious;
- investigate;
- pursue;
- search;
- alarm;
- return-to-duty states.

### Rival Operatives
Use goal-driven high-level objectives feeding StateTree tactical behaviour.

Do not initially build a sophisticated general-purpose planner unless simpler goal selection proves insufficient.

### City Dressing
MassEntity may later support large numbers of lightweight ambient actors.

Because civilians are non-tactical scenery, **Mass should not be a first-playable dependency**.

---

# 50. Animation Implementation

Use existing root-motion / animation assets as the foundation.

Potential UE systems include:
- motion matching / pose selection where justified;
- distance matching;
- pose warping;
- Motion Warping;
- contextual animation.

For the first playable:
- conventional animation graph;
- root-motion contextual actions;
- conservative alignment;
- simple validated interaction sockets

are preferable to a large experimental traversal framework.

---

# 51. Major Technical Risks

## Flight-Speed Streaming

**Risk:** aircraft can cross streaming boundaries much faster than a walking open-world character.

**Mitigation:**
- build flight traversal early;
- use representative asset density immediately;
- test World Partition/HLOD from the first playable;
- favour skyline readability over dense small geometry.

## Parkour Reliability

**Risk:** store animations do not automatically produce reliable contextual traversal.

**Mitigation:**
- small traversal verb set;
- strict level metrics;
- authored parkour routes;
- traversal validation tools;
- no optional movement verb until it proves reliable.

## Mission Proceduralism

**Risk:** procedural combinations create unreadable or trivial stealth states.

**Mitigation:**
- authored sockets;
- tagged compatibility;
- validator;
- deterministic mission seeds;
- automated reachability tests;
- curated configuration rules.

## Alert-State Complexity

**Risk:** fast movement makes AI appear psychic or incompetent.

**Mitigation:**

Track explicitly:
- who saw the player;
- last known position;
- information age;
- who has received that information;
- search state.

Never conflate `site knows intruder exists` with `every guard knows exact player transform`.

## Aircraft / Mission Interaction

**Risk:** an aircraft physically above every mission complicates rendering, navigation, weapons and streaming.

**Mitigation:**

Treat support behaviour as controlled states rather than full unscripted aircraft autonomy while the player is on foot.

## Art Shader

**Risk:** comic post-process looks attractive in screenshots but destroys stealth readability.

**Mitigation:**

Prototype visual treatment against:
- enemy silhouettes;
- dark interiors;
- neon signs;
- depth perception;
- flight-speed scenery;
- detection indicators.

Shader approval should depend on gameplay tests, not still images.

---

# 52. First Playable

The first playable must prove a miniature version of the **whole game loop**.

## World

One small flyable district.

Contains:
- one controlling gang;
- police presence;
- one corporate relationship;
- one mission site.

## Flight

Must support:
- take-off / free flight;
- arcade VTOL handling;
- scan;
- mission approach;
- hover;
- cloak;
- insertion;
- extraction;
- departure.

No dogfighting required.

## Ground

Must support:
- third-person movement;
- contextual parkour;
- perception mode;
- one contextual takedown;
- one firearm;
- basic hack;
- layered detection;
- local search;
- one objective.

## Procedural Variation

At least 2–3 contract configurations using the same mission arena.

Variation should include some combination of:
- target;
- guard positions;
- patrol;
- security device;
- insertion;
- extraction.

## Rival Escalation

At least one rival operative intervention triggered by overstaying.

Aircraft compromise can be simplified but must exist end-to-end.

## World Consequence

At least one success/failure result changes persistent world state or changes the next available contract.

## Police

Basic Authority/Heat behaviour must exist.

---

# 53. First-Playable Success Criteria

The build succeeds if playtesting demonstrates all of the following:

### Ground fantasy
Players voluntarily use perception mode to make decisions rather than simply as generic bullet-time.

### Speed fantasy
Competent players regularly execute short, fluid parkour/stealth bursts.

### Detection recovery
A brief sighting frequently leads to successful repositioning rather than inevitable combat.

### Mission pressure
Players understand that remaining on-site increases rival danger.

### Aircraft integration
The flight, cloak, insertion, ground mission and extraction sequence feels like one loop rather than separate prototypes.

### Procedural value
The same arena produces recognisably different tactical problems across configurations.

### Failure
A failed mission can transition cleanly into a valid next world/campaign state.

### Scope
None of the above requires deep combat, civilian simulation, procedural level geometry, multiplayer or bespoke animation production.

---

# 54. Post-First-Playable Vertical Slice

Only after the first playable succeeds should production expand toward a vertical slice.

Suggested additions:
- second distinct mission arena;
- second district;
- second rival corporate doctrine;
- multiple cyberware loadouts;
- aircraft progression;
- stronger comic rendering;
- basic corporate sponsor system;
- small HTN campaign chain;
- one authored major narrative block;
- territory transition;
- hot extraction with polished aircraft fire support.

Dogfighting remains optional.

---

# 55. Content Production Targets

Do not lock final counts until production velocity is measured from the first playable.

Current directional target:
- 6–8 city districts;
- limited reusable environment kits;
- several authored mission arenas per broad environment family;
- 5–6 mission archetypes;
- ~5 enemy behavioural archetypes;
- small shared security-device library;
- ~6–8 cyberware abilities;
- 2–3 equipped cyberware slots;
- compact weapon roster;
- several rival corporation doctrines;
- one police doctrine with district variations;
- reusable routine narrative-block library;
- smaller number of high-quality authored campaign beats.

The number of **validated mission configurations** matters more than the number of raw maps.

---

# 56. Key Production Rule

Every major feature should be challenged with:

> Does this materially strengthen the fantasy of being an impossibly fast corporate operative who flies into a contested city, performs short high-speed infiltrations, and escapes before competing systems can close around them?

If not, remove it or defer it.

---

# 57. Open Tunables

These should be playtested rather than decided in the GDD:
- precise perception duration;
- precise perception recharge;
- cloak duration;
- rival trace timings;
- rival insertion timing;
- final aircraft hack duration;
- detection distances;
- search duration;
- player lethality;
- player survivability;
- police escalation thresholds;
- district traversal size;
- aircraft maximum speed;
- aircraft acceleration/braking;
- mission reward pacing.

---

# 58. Remaining Design Questions

The principal unanswered questions are now implementation/content questions rather than foundational design questions:

1. Exact city fiction, corporations and gang identities.
2. Player corporation's starting doctrine.
3. Initial cyberware set.
4. Aircraft visual design and weapon/support hardpoints.
5. First mission arena theme.
6. Exact objective set for the first playable.
7. Economy/currency presentation.
8. Exact save implementation for ironman-lite campaign persistence.
9. Campaign block data schema.
10. Shader prototype viability.

These should be resolved through prototyping and content design rather than another broad concept grill.

---

# 59. One-Sentence Production Definition

**A compact single-player cyberpunk stealth-action game in which a corporate operative flies freely through a vertically contested city, launches from a cloaked VTOL into short procedurally configured stealth missions, uses brief perception slowdown to plan extremely fast parkour attacks, and must finish before rival corporations locate and hack the aircraft overhead.**
