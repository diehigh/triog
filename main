#include <Windows.h>
#include <iostream>
#include "HackProcess.h"
#include "Offsets.h"
#include "Settings.h"
#include "Weapon.h"
#include "Utils.h"
#include "Vector.h"

CHackProcess fProcess;

using namespace std;

bool b_true = true;
bool b_false = false;

bool triggerbotEnabled = true; // Whether the triggerbot is enabled or not

bool attackState = false; // Whether +attack or -attack was last pressed (false = -attack)
bool slept = false; // Whether we slept or not (so it doesn't shoot retardedly)

bool useCoords = true; // Whether to use coordinate based triggerbot or not
bool useCeid = true; // Whether to use ceid based trigger bot or not
bool msgSent = false; // Whether the warning about mp_playerid was sent or not

RECT m_Rect; // Rectangle for the game window

// A world to screen (view) matrix.
typedef struct
{
	float flMatrix[4][4];
}WorldToScreenMatrix;

// My player.
// A wrapper for reading his information from memory and storing it.
struct MyPlayer
{
	DWORD CLocalPlayer; // Base address of player info in memory
	DWORD CViewMatrix;  // Base address of view matrix in memory
	DWORD CWeaponBase;  // Weapon base address in memory
	int CrosshairEntityId; // Stores m_flags of player
	int Team; // The team the player is on
	Vector Position;
	int WeaponID;
	Vector Velocity;
	WorldToScreenMatrix WorldToScreenMatrix;

	// Read information about the player from memory
	void ReadInfo()
	{
		// Read team id
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(fProcess.__dwordClient+dw_player_base),
			&CLocalPlayer, sizeof(DWORD), 0);
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CLocalPlayer+dw_crosshair_id_offset),
			&CrosshairEntityId, sizeof(int), 0);
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CLocalPlayer+dw_team_num_offset),
			&Team, sizeof(int), 0);

		ReadViewMatrix();
		ReadWeaponId();
	}

	// Read view matrix
	void ReadViewMatrix()
	{
		CViewMatrix = fProcess.__dwordClient+dw_view_matrix_base;
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CViewMatrix+dw_local_view_offset),
			&Position, sizeof(Position), 0);
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CViewMatrix),
			&WorldToScreenMatrix, sizeof(WorldToScreenMatrix), NULL);
	}

	// Reads the player's current weapon id.
	void ReadWeaponId()
	{
		int WeaponHandle;
		int WeaponIDFirst;

		// Read weapon id
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CLocalPlayer+dw_m_h_ActiveWeapon),
			&WeaponHandle, 4, NULL);
		WeaponIDFirst = WeaponHandle & 0xFFF;
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(fProcess.__dwordClient+dw_entity_base+(-0x10+(WeaponIDFirst*0x10))),
			&CWeaponBase, 4, NULL);
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CWeaponBase+dw_weapon_id),
			&WeaponID, 4, NULL);
	}
}MyPlayer;

// Wrapper around the player list. Use ReadInformation to store
// the base address of a player using his CEID and his team.
struct PlayerList_t
{
	DWORD CBaseEntity;
	int Team;
	Vector Position;

	void ReadInformation(int ceid)
	{
		// Offset from client.dll. Gets to the player list base address
		// and then to the player base address
		DWORD offset = dw_entity_base+((ceid - 1) * dw_entity_size);
		// Store the player's base address
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(fProcess.__dwordClient+offset),
			&CBaseEntity, sizeof(DWORD), 0);
		// Read the team of the player from his base address in the player list
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CBaseEntity+dw_team_num_offset),
			&Team, sizeof(int), 0);
		// Read his location
		ReadProcessMemory(fProcess.__HandleProcess,
			(PBYTE*)(CBaseEntity+dw_origin_offset),
			&Position, sizeof(Position), 0);
	}
}PlayerList[32];

FLOAT fScreenX, fScreenY;
BOOL ScreenTransform(CONST Vector& point, OUT Vector& screen)
{
	float worldToScreen[4][4];
	memcpy(worldToScreen, MyPlayer.WorldToScreenMatrix.flMatrix,
		sizeof(worldToScreen));

	screen.X = worldToScreen[0][0]
	* point.X + worldToScreen[0][1]
	* point.Y + worldToScreen[0][2]
	* point.Z + worldToScreen[0][3];
	screen.Y = worldToScreen[1][0]
	* point.X + worldToScreen[1][1]
	* point.Y + worldToScreen[1][2]
	* point.Z + worldToScreen[1][3];

	FLOAT w;
	w = worldToScreen[3][0]
	* point.X + worldToScreen[3][1]
	* point.Y + worldToScreen[3][2]
	* point.Z + worldToScreen[3][3];
	screen.Z = 0.0f;

	BOOL behind = FALSE;
	if(w < 0.001f)
	{
		behind = TRUE;
		screen.X *= 100000;
		screen.Y *= 100000;
	}
	else
	{
		FLOAT invw = 1.0f / w;
		screen.X *= invw;
		screen.Y *= invw;
	}

	return behind;
}

// Take a 3d point and turn it into 2d coordinates on the screen
BOOL WorldToScreen(CONST Vector &origin, OUT Vector &screen)
{
	BOOL bRet = FALSE;

	if(!ScreenTransform(origin, screen))
	{
		INT width = (int)(m_Rect.right - m_Rect.left);
		INT height = (int)(m_Rect.bottom - m_Rect.top);

		fScreenX = width / 2.0f;
		fScreenY = height / 2.0f;

		fScreenX += 0.5f * screen.X * width + 0.5f;
		fScreenY -= 0.5f * screen.Y * height + 0.5f;

		screen.X = fScreenX;
		screen.Y = fScreenY;

		bRet = TRUE;
	}

	return bRet;
}

// Gets the value of mp_playerid.
int mp_playerid()
{
	if (!useCeid)
		return 1;

	int out;
	ReadProcessMemory(fProcess.__HandleProcess,
		(PBYTE*)(fProcess.__dwordClient+dw_mp_playerid),
		&out, sizeof(int), 0);
	if (!msgSent && out > 0)
	{
		cout << "WARNING: mp_playerid is nonzero. "
			<< "This may affect triggerbot accuracy, as boundingbox will be used"
			<< endl;
		msgSent = true;
	}
	return out;
}

// Gets the number of players in the game
int NumberOfPlayers()
{
	return 32;
}

// Fires the current weapon once.
void Attack()
{
	if (!attackState) // +attack
	{
		// +attack/-attack method
		//WriteProcessMemory(fProcess.__HandleProcess,
		//	(PBYTE*)(fProcess.__dwordClient+dw_attack),
		//	&b_true, sizeof(bool), NULL);

		// mouse_event method
		mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,0);
		attackState = true;
		//cout << "+attack" << endl;
	}
	else // -attack
	{
		// +attack/-attack method
		//WriteProcessMemory(fProcess.__HandleProcess,
		//	(PBYTE*)(fProcess.__dwordClient+dw_attack),
		//	&b_false, sizeof(bool), NULL);

		// mouse_event method
		mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,0);
		attackState = false;
		//cout << "-attack" << endl;
	}
	Sleep(1);
}

void CheckKeybinds()
{
	// Check for toggle
	if(GetAsyncKeyState(VK_F7))
	{
		triggerbotEnabled = !triggerbotEnabled;
		cout << "Triggerbot toggled to " << triggerbotEnabled << endl;
		Sleep(500);
	}
	if(GetAsyncKeyState(VK_F11))
	{
		useCoords = !useCoords;
		cout << "Warning: Co-ordinate triggerbot forced to " << useCoords << endl;
		cout << "This will allow for scoped weapons to work while "
			<< "scoped but will decrease accuracy." << endl;
		Sleep(500);
	}
	if(GetAsyncKeyState(VK_F8))
	{
		useCeid = !useCeid;
		cout << "Warning: InCross triggerbot forced to " << useCeid << endl;
		cout << "This will allow for the triggerbot to work if it mp_playerid "
			<< "1/2 wasn't detectec automatically for some reason." << endl;
		cout << "This will drastically decrease the accuracy of your aimbot as "
			<< "it will be forced to use the bounding box of the player." << endl;
		Sleep(500);
	}
}

int main()
{
	// Random window title
	char text[32];
	GetRandomAlphanumeric(text, 32);
	SetConsoleTitle(text);

	if (!SHOW_CONSOLE)
	ShowWindow(FindWindow("ConsoleWindowClass", NULL), false);

	cout << "Searching for CS:GO..." << endl;
	fProcess.RunProcess(); // Wait for CS:GO
	cout << "CS:GO Found" << endl;

	if (!GetWindowRect(fProcess.__HWNDCsgo, &m_Rect))
		return 1;
	int halfWidth = (int)(m_Rect.right - m_Rect.left) / 2; // Half of the game window's width
	int halfHeight = (int)(m_Rect.bottom - m_Rect.top) / 2; // Half of the game window's height

	// Main loop
	for (; ;)
	{
		Sleep(1);

		CheckKeybinds();

		// Make sure -attack was used if the weapon isn't full auto
		if (attackState && IsWeaponSemiAuto(MyPlayer.WeaponID))
			Attack();

		// When mouse5 is held down and the triggerbot is on
		if (!(triggerbotEnabled && GetAsyncKeyState(VK_XBUTTON2)))
		{
			if (attackState) // -attack if we are +attack and we release mouse5
				Attack();
			continue;
		}

		MyPlayer.ReadInfo(); // Update player information
		// If our weapon is not non-aim (it's a gun)
		if (IsWeaponNonAim(MyPlayer.WeaponID))
			continue;

		bool hit = false;
		for (int i = 1; i <= NumberOfPlayers(); ++i)
		{
			PlayerList[i].ReadInformation(i);
			// If our crosshair is on a player... (ceid has to be less than
			// number of players or it might be on something like a flashbang)
			if (!((MyPlayer.CrosshairEntityId > 0
				&& MyPlayer.CrosshairEntityId <= NumberOfPlayers())
				|| mp_playerid() > 0)) {
				// Use -attack if we would miss and the weapon is full auto
				if (attackState && !IsWeaponSemiAuto(MyPlayer.WeaponID))
					Attack();
				continue;
			}

			// Get the information of the player we have our crosshair on
			PlayerList[i].ReadInformation(i);
			if (MyPlayer.Team == PlayerList[i].Team && !ALLOW_FRIENDLYFIRE)
				continue;

			bool shouldShoot = true; // Whether we should shoot or not
			// Only use coord based triggerbot if our weapon doesn't have a scope
			if (useCoords && !IsWeaponScoped(MyPlayer.WeaponID))
			{
				shouldShoot = false;
				Vector ScreenPos;
				// If the enemy is on our screen...
				if(WorldToScreen(PlayerList[i].Position, ScreenPos))
				{
					// Do all this shit to get a general box around him
					double distance = Get3dDistance(MyPlayer.Position, PlayerList[i].Position);
					int playerWidth = (int)max(PLAYER_WIDTH / distance, 11);
					int playerHeight = (int)max(PLAYER_HEIGHT / distance, 5);
					int halfPlayerWidth = (int)(playerWidth / 2);
					int heightOffset = (int)(PLAYER_HEIGHT_OFFSET / distance);

					// Check if the crosshair's in the box
					if(ScreenPos.X > halfWidth - halfPlayerWidth
						&& ScreenPos.X < halfWidth + halfPlayerWidth
						&& ScreenPos.Y > halfHeight + heightOffset
						&& ScreenPos.Y < halfHeight + playerHeight + heightOffset)
						shouldShoot = true;
				}
			}
			if (shouldShoot)
			{
				hit = true;
				if (DELAY > 0)
				{
					Sleep(DELAY);
					slept = true;
				}
				if(!attackState)
					Attack();
				Sleep(1);
				break;
			}
		}
		if (!hit && attackState && !IsWeaponSemiAuto(MyPlayer.WeaponID))
			Attack();
		
		if (slept)
		{
			Sleep(DELAY);
			slept = false;
		}
		Sleep(1);
	}
	return 0;
}
