# Mission state stays a UWorldSubsystem

`AEOGameModeBase` carried a comment claiming mission logic lives in
`UEOMissionSubsystem` "so it survives level transitions". That is not what a
`UWorldSubsystem` does — it is created and destroyed with the world. The comment
described a property the code never had.

Resolved in favour of the code, not the comment: a mission is per-world by
definition, and a new level means a new mission. The comment has been corrected.

Recorded so nobody "fixes" this by promoting it to a `UGameInstanceSubsystem`.
Revisit only when something genuinely has to outlive a world — a campaign, a hub
level, or persistent progression between missions.
