#pragma once

// Called after the player toggles the per-level ghost switch in the popup,
// so the change takes effect immediately instead of waiting for the next
// level load.
void refreshLiveGhostState();
