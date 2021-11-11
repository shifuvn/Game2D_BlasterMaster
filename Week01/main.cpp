#include "resources.h"
#include "GameObject.h"
#include "Textures.h"
#include "GamePlayer.h"
#include "GameNpc.h"

CGame* game;
CGamePlayer* player;
CGameNpc* npc;
CTextures* textures = CTextures::Get_instance();
CSprites* sprites = CSprites::Get_instance();
CAnimations* animations = CAnimations::Get_instance();

class CKeyHander : public CKeyEventHandler
{
	virtual void Key_state(BYTE* states);
	virtual void On_key_down(int KeyCode);
	virtual void On_key_up(int KeyCode);
};

CKeyHander* keyHandler;

void CKeyHander::On_key_down(int KeyCode)
{
	DebugOut(L"[INFO] KeyDown: %d\n", KeyCode);
}

void CKeyHander::On_key_up(int KeyCode)
{
	DebugOut(L"[INFO] KeyUp: %d\n", KeyCode);
}

void CKeyHander::Key_state(BYTE* states)
{
	if (game->Is_key_down(DIK_RIGHT)) {
		player->Set_state(PLAYER_STATE_MOVING_RIGHT);
	}
	else if (game->Is_key_down(DIK_LEFT)) {
		player->Set_state(PLAYER_STATE_MOVING_LEFT);
	}
	else if (game->Is_key_down(DIK_UP)) {
		player->Set_state(PLAYER_STATE_MOVING_UP);
	}
	else if (game->Is_key_down(DIK_DOWN)) {
		player->Set_state(PLAYER_STATE_MOVING_DOWN);
	}
	else player->Set_state(PLAYER_STATE_IDLE);
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

void Add_mario_sprites(LPDIRECT3DTEXTURE9 texture) {
	// mario move left
	sprites->Add(10001, 246, 154, 260, 181, texture);
	sprites->Add(10002, 275, 154, 290, 181, texture);
	sprites->Add(10003, 304, 154, 321, 181, texture);

	// mario move right
	sprites->Add(10011, 186, 154, 200, 181, texture);
	sprites->Add(10012, 155, 154, 170, 181, texture);
	sprites->Add(10013, 125, 154, 140, 181, texture);

	// mario move up
	sprites->Add(10021, 215, 194, 232, 222, texture);
	sprites->Add(10022, 92, 275, 111, 300, texture);
	sprites->Add(10023, 335, 275, 351, 300, texture);

	// mario move down
	sprites->Add(10031, 215, 154, 231, 181, texture);
	sprites->Add(10032, 215, 234, 231, 251, texture);
}

void Add_mario_animations() {
	LPANIMATION lpani;

	lpani = new CAnimation(100);
	lpani->Add(10001);
	animations->Add(400, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10011);
	animations->Add(401, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10021);
	animations->Add(402, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10031);
	animations->Add(403, lpani);
	//
	lpani = new CAnimation(100);
	lpani->Add(10001);
	lpani->Add(10002);
	lpani->Add(10003);
	animations->Add(500, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10011);
	lpani->Add(10012);
	lpani->Add(10013);
	animations->Add(501, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10021);
	lpani->Add(10022);
	lpani->Add(10023);
	animations->Add(502, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10031);
	lpani->Add(10032);
	animations->Add(503, lpani);
}

void Add_npc_animations() {
	LPANIMATION lpani;
	lpani = new CAnimation(100);
	lpani->Add(10001);
	lpani->Add(10002);
	lpani->Add(10003);
	animations->Add(600, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10011);
	lpani->Add(10012);
	lpani->Add(10013);
	animations->Add(601, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10021);
	lpani->Add(10022);
	lpani->Add(10023);
	animations->Add(602, lpani);

	lpani = new CAnimation(100);
	lpani->Add(10031);
	lpani->Add(10032);
	animations->Add(603, lpani);
}

/*
	Load all game resources
	In this example: load textures, sprites, animations and mario object
*/
void Load_resources()
{
	textures->Add(ID_TEXTURES_PLAYER, MARIO_TEXTURE_PATH, TEXTURE_TRANS_COLOR);
	textures->Add(ID_TEXTURES_NPC, MARIO_TEXTURE_PATH, TEXTURE_TRANS_COLOR);
	LPDIRECT3DTEXTURE9 texPlayer = textures->Get(ID_TEXTURES_PLAYER);
	LPDIRECT3DTEXTURE9 texNpc = textures->Get(ID_TEXTURES_NPC);

	Add_mario_sprites(texPlayer);
	Add_mario_sprites(texNpc);
	Add_mario_animations();
	Add_npc_animations();

	player = new CGamePlayer();
	CGamePlayer::Add_animation(400);		// idle right
	CGamePlayer::Add_animation(401);		//		left
	CGamePlayer::Add_animation(402);		//		up
	CGamePlayer::Add_animation(403);		//		down
	CGamePlayer::Add_animation(500);		// walk right
	CGamePlayer::Add_animation(501);		//		left
	CGamePlayer::Add_animation(502);		//		up
	CGamePlayer::Add_animation(503);		//		down

	npc = new CGameNpc();
	CGameNpc::Add_animation(600);
	CGameNpc::Add_animation(601);
	CGameNpc::Add_animation(602);
	CGameNpc::Add_animation(603);

	player->Set_position(PLAYER_START_X, PLAYER_START_Y);
	npc->Set_position(NPC_START_X, NPC_START_Y);
	npc->Set_speed(NPC_MOVING_SPEED, 0);
}

/*
	Update world status for this frame
	dt: time period between beginning of last frame and beginning of this frame
*/
void Update(DWORD dt)
{
	player->Update(dt);
	npc->Update(dt);
}

/*
	Render a frame
*/
void Render()
{
	LPDIRECT3DDEVICE9 d3ddv = game->Get_direct3D_device();
	LPDIRECT3DSURFACE9 bb = game->Get_backbuffer();
	LPD3DXSPRITE spriteHandler = game->Get_sprite_handler();

	if (d3ddv->BeginScene())
	{
		// Clear back buffer with a color
		d3ddv->ColorFill(bb, NULL, BACKGROUND_COLOR);

		spriteHandler->Begin(D3DXSPRITE_ALPHABLEND);

		player->Render();
		npc->Render();

		spriteHandler->End();
		d3ddv->EndScene();
	}

	// Display back buffer content to the screen
	d3ddv->Present(NULL, NULL, NULL, NULL);
}

HWND Create_game_window(HINSTANCE hInstance, int nCmdShow, int ScreenWidth, int ScreenHeight)
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hInstance;

	wc.lpfnWndProc = (WNDPROC)WinProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS_NAME;
	wc.hIconSm = NULL;

	RegisterClassEx(&wc);

	HWND hWnd =
		CreateWindow(
			WINDOW_CLASS_NAME,
			WINDOW_TITLE,
			WS_OVERLAPPEDWINDOW, // WS_EX_TOPMOST | WS_VISIBLE | WS_POPUP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			ScreenWidth,
			ScreenHeight,
			NULL,
			NULL,
			hInstance,
			NULL);

	if (!hWnd)
	{
		OutputDebugString(L"[ERROR] CreateWindow failed");
		DWORD ErrCode = GetLastError();
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

int Run()
{
	MSG msg;
	int done = 0;
	DWORD frameStart = GetTickCount();
	DWORD tickPerFrame = 1000 / MAX_FRAME_RATE;

	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) done = 1;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		DWORD now = GetTickCount();

		// dt: the time between (beginning of last frame) and now
		// this frame: the frame we are about to render
		DWORD dt = now - frameStart;

		if (dt >= tickPerFrame)
		{
			frameStart = now;

			game->Process_keyboard();

			Update(dt);
			Render();
		}
		else
			Sleep(tickPerFrame - dt);
	}

	return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd = Create_game_window(hInstance, nCmdShow, SCREEN_WIDTH, SCREEN_HEIGHT);

	game = CGame::Get_instance();
	game->Init_game(hWnd);

	keyHandler = new CKeyHander();
	game->Init_keyboard(keyHandler);


	Load_resources();
	Run();

	return 0;
}