@{
	GhostEluna = @(
		@{ Id = 'GE-01'; Description = 'Ghost LMB tracer and impact' }
		@{ Id = 'GE-02'; Description = 'Ghost RMB flight and detonation radius' }
		@{ Id = 'GE-03'; Description = 'Ghost Shift trail and synchronized movement' }
		@{ Id = 'GE-04'; Description = 'Ghost Q piercing line and per-target burst' }
		@{ Id = 'GE-05'; Description = 'Ghost R warning, active floor, and damage radius' }
		@{ Id = 'GE-06'; Description = 'Ghost passive kill reset pulse and Shift refresh' }
		@{ Id = 'GE-07'; Description = 'Eluna held LMB readability and damage stacking' }
		@{ Id = 'GE-08'; Description = 'Eluna RMB projectile, root growth, pop, damage, and root' }
		@{ Id = 'GE-09'; Description = 'Eluna Shift path, ally pass-through, pulse, and refund' }
		@{ Id = 'GE-10'; Description = 'Eluna Q travel, attachment, exact radius, pulses, and final burst' }
		@{ Id = 'GE-11'; Description = 'Eluna R narrow tether, movement tracking, and all cleanup cases' }
		@{ Id = 'GE-12'; Description = 'Eluna passive heal pulse and wisp pickup/drop readability' }
	)
	KingpinHudson = @(
		@{ Id = 'KH-01'; Description = 'Kingpin LMB footprint, impact, and one chain line' }
		@{ Id = 'KH-02'; Description = 'Kingpin RMB hook, latch, single pull, and cleanup' }
		@{ Id = 'KH-03'; Description = 'Kingpin Shift direction, dash, and collision bursts' }
		@{ Id = 'KH-04'; Description = 'Kingpin Q sector and gameplay cone agreement' }
		@{ Id = 'KH-05'; Description = 'Kingpin R two sectors, per-shell hits, and anti-heal' }
		@{ Id = 'KH-06'; Description = 'Kingpin passive response to cooldown-shortening CC' }
		@{ Id = 'KH-07'; Description = 'Hudson RMB progress, spun-up state, release, and wind-down' }
		@{ Id = 'KH-08'; Description = 'Hudson ten-second normal/spun-up LMB and cue stability' }
		@{ Id = 'KH-09'; Description = 'Hudson Shift trail, collision burst, release, and hover end' }
		@{ Id = 'KH-10'; Description = 'Hudson Q mesh visibility, color, orientation, and hit box' }
		@{ Id = 'KH-11'; Description = 'Hudson R phases and miss/timeout/death cleanup' }
		@{ Id = 'KH-12'; Description = 'Hudson passive repair/heal pulse readability' }
	)
	CrystaVoid = @(
		@{ Id = 'CV-01'; Description = 'Crysta LMB impact and aligned empowered projectiles' }
		@{ Id = 'CV-02'; Description = 'Crysta Reverberation application and detonation feedback' }
		@{ Id = 'CV-03'; Description = 'Crysta RMB curve, state colors, release return, and pull' }
		@{ Id = 'CV-04'; Description = 'Crysta Shift charge trails and empowered LMB grant' }
		@{ Id = 'CV-05'; Description = 'Crysta Q exact radius and one recast/expiry burst' }
		@{ Id = 'CV-06'; Description = 'Crysta R full lanes, alternating X/V beams, and final pair' }
		@{ Id = 'CV-07'; Description = 'Void normal/charged LMB and wall-piercing pawn hit' }
		@{ Id = 'CV-08'; Description = 'Void passive build-up and empowered consumption' }
		@{ Id = 'CV-09'; Description = 'Void RMB cone orientation, hitbox tracking, and empowered size' }
		@{ Id = 'CV-10'; Description = 'Void Shift endpoint zones, bursts, and eligible actor swaps' }
		@{ Id = 'CV-11'; Description = 'Void Q exact radii, ticks, expiry burst, and recast burst' }
		@{ Id = 'CV-12'; Description = 'Void R warning, pull, stun, active state, implosion, and cleanup' }
	)
	CleanupStress = @(
		@{ Id = 'CS-01'; Description = 'Kill each caster during the listed persistent abilities' }
		@{ Id = 'CS-02'; Description = 'No stale visual, timer, movement, hook, or empowered state remains' }
		@{ Id = 'CS-03'; Description = 'Hudson sustained fire and persistent-zone overlap under 100 ms/2% loss' }
		@{ Id = 'CS-04'; Description = 'Final candidate presentation on Low and Medium effects quality' }
		@{ Id = 'CS-05'; Description = 'Grappling Hook owner/observer endpoint, pull, range, and cleanup' }
	)
	RangeIndicators = @(
		@{ Id = 'RI-01'; Description = 'All hunter hover rings are owner-only and singular' }
		@{ Id = 'RI-02'; Description = 'Held LMB/RMB aim line follows cursor and clears on release' }
		@{ Id = 'RI-03'; Description = 'Shift/Q/R preview is bounded and clears on pawn lifecycle changes' }
		@{ Id = 'RI-04'; Description = 'Ground target centers clamp while footprints remain centered' }
		@{ Id = 'RI-05'; Description = 'Void Q footprint extends beyond the clamped cast ring' }
		@{ Id = 'RI-06'; Description = 'F/G metadata follows equipped powers and slot swaps' }
		@{ Id = 'RI-07'; Description = 'Tactical Nuke preview persists through confirm or cancel' }
		@{ Id = 'RI-08'; Description = 'Cooldown rejection blocks live preview but not hover preview' }
		@{ Id = 'RI-09'; Description = 'Range previews preserve Hudson and Crysta RMB release behavior' }
	)
	WispUI = @(
		@{ Id = 'WI-01'; Description = 'Two replicated bars are readable without overlap or clipping' }
		@{ Id = 'WI-02'; Description = 'Natural master decay is smooth and revive stays empty' }
		@{ Id = 'WI-03'; Description = 'Ally/healing freezes decay and fills revive at the correct rate' }
		@{ Id = 'WI-04'; Description = 'Enemy decay visibly overrides ally proximity and healing' }
		@{ Id = 'WI-05'; Description = 'Eluna carry/CC drop priority and bar lifecycle are readable' }
	)
}
