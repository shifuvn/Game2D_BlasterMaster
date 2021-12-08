#include "Game.h"
#include "Animations.h"
#include "Camera.h"
#include "KeyEventHandler.h"
#include "InputHandler.h"
#include "QuadTree.h"
#include "Textures.h"

#include "Sophia.h"
#include "Jason.h"
#include "Brick.h"
#include "EnemyDrap.h"
#include "EnemyEyelet.h"
#include "EnemyGX-680.h"
#include "EnemyOffside.h"
#include "EnemyStuka.h"
#include "EnemyBallot.h"

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/filereadstream.h"

CSophia* sophia;
CJason* jason;
CCamera* camera = CCamera::GetInstance();
CQuadTree* quadtree;

std::vector<CGameObject*> worldObjects;
std::vector<CGameObject*> renderedObjects;

CTextures* g_textures = CTextures::GetInstance();
CSprites* g_sprites = CSprites::GetInstance();
CAnimations* g_animations = CAnimations::GetInstance();
CInputHandler* g_inputHandler = CInputHandler::GetInstance();

CGame* CGame::__instance = nullptr;
DWORD CGame::dt = 0;

#pragma region Keyboard

class CKeyHander : public CKeyEventHandler
{
	virtual void KeyState(BYTE* states);
	virtual void OnKeyDown(int KeyCode);
	virtual void OnKeyUp(int KeyCode);
	virtual void IsKeyDown(int keyCode);
};

void CKeyHander::OnKeyDown(int keyCode)
{
	
}

void CKeyHander::IsKeyDown(int keyCode) {

}

void CKeyHander::OnKeyUp(int keyCode)
{
}

void CKeyHander::KeyState(BYTE* states)
{
	// switch selected object by key
	// empty
}

#pragma endregion

#pragma region DirectX

void CGame::InitDirectX(HWND hWnd)
{
	LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);

	this->hWnd = hWnd;

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;

	RECT r;
	GetClientRect(hWnd, &r);	// retrieve Window width & height 

	d3dpp.BackBufferHeight = r.bottom + 1;
	d3dpp.BackBufferWidth = r.right + 1;

	mapWidth = d3dpp.BackBufferWidth;
	mapHeight = d3dpp.BackBufferHeight;

	d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddv);

	if (d3ddv == NULL)
	{
		OutputDebugString(L"[ERROR] CreateDevice failed\n");
		return;
	}

	d3ddv->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

	// Initialize sprite helper from Direct3DX helper library
	D3DXCreateSprite(d3ddv, &spriteHandler);

	DebugOut(L"[INFO] InitGame done;\n");
	DebugOut(L"[INFO] Screen: %d, %d\n", mapWidth, mapHeight);
}

/*
	Utility function to wrap LPD3DXSPRITE::Draw
*/
void CGame::Draw(Vector2D position, int nx, LPDIRECT3DTEXTURE9 texture, int left, int top, int right, int bottom, int alpha)
{

	Vector2D cameraPos = camera->GetPosition();

	Vector3D p(0, 0, 0);
	RECT r;
	r.left = left;
	r.top = top;
	r.right = right;
	r.bottom = bottom;

	Vector3D center = Vector3D((right - left) / 2, (bottom - top) / 2, 0.0f);

	D3DXMATRIX matrix;
	D3DXMatrixIdentity(&matrix);

	// flip X
	D3DXMATRIX dmFlipX;
	D3DXMatrixScaling(&dmFlipX, nx, 1.0f, 1.0f);

	// translate 
	D3DXMATRIX dmTranslation;
	D3DXMatrixTranslation(&dmTranslation, (position.x - cameraPos.x), (-position.y + cameraPos.y), 0.0f);

	matrix *= dmFlipX;
	matrix *= dmTranslation;

	spriteHandler->SetTransform(&matrix);

	spriteHandler->Draw(texture, &r, &center, &p, D3DCOLOR_ARGB(alpha, 255, 255, 255));
}

#pragma endregion

#pragma region Game process

void CGame::InitGame(HWND hWnd)
{
	this->InitDirectX(hWnd);

	#pragma region TEXTURES FOR TEST ONLY
	
	// For only test
	g_textures->Add(10, SOPHIA_JASON_TEXTURE_PATH, D3DCOLOR_XRGB(0, 0, 0));
	LPDIRECT3DTEXTURE9 texTestDebug = g_textures->Get(10);
	g_sprites->Add(2000, 0, 0, 1, 1, texTestDebug);

	g_textures->Add(20, L"textures//bbox.png", D3DCOLOR_XRGB(0, 0, 0));

	#pragma endregion

	this->LoadResource();

	// set controller object
	auto controller = CControllerObject::GetInstance();
	controller->SetSophiaAndJason(sophia, jason);
	controller->Select(ControllerObjectID::SOPHIA);
	
	// start keyboard
	#pragma region KEYBOARD
	
	this->keyHandler = new CKeyHander();
	g_inputHandler->SetHandleWindow(this->hWnd);
	g_inputHandler->SetKeyHandler(keyHandler);
	g_inputHandler->InitKeyboard();

	#pragma endregion
}

void CGame::UpdateGame(DWORD dt)
{
	camera->Update();
	quadtree->Update(worldObjects);
	renderedObjects.clear();
	quadtree->Retrieve(renderedObjects, camera->GetBoundingBox());
	
	for (auto object : renderedObjects) {
		if (object->IsActive() == true) {
			object->FilterTriggerTag();
			object->PhysicalUpdate(&renderedObjects);
		}
	}

	for (auto object : renderedObjects) {
		if (object->IsActive() == true) {
			object->Update(dt);
		}
	}
}

void CGame::RenderGame()
{
	LPDIRECT3DDEVICE9 d3ddv = CGame::GetInstance()->GetDirect3dDevice();
	LPDIRECT3DSURFACE9 bb = CGame::GetInstance()->GetBackbuffer();
	LPD3DXSPRITE spriteHandler = CGame::GetInstance()->GetSpriteHandler();

	if (d3ddv->BeginScene())
	{
		// Clear back buffer with a color
		d3ddv->ColorFill(bb, NULL, BACKGROUND_COLOR);

		spriteHandler->Begin(D3DXSPRITE_ALPHABLEND);

		this->map->Draw(Vector2D(this->mapWidth / 2, this->mapHeight / 2), 1, 255);
		for (auto object : renderedObjects) {
			if (!object->IsLive() || !object->IsActive()) {
				continue;
			}

			object->Render();
		}

		for (auto object : renderedObjects) {
			if (!object->IsActive()) {
				continue;
			}

			for (auto co : object->GetColliders()) {
				co->RenderBoundingBox();
			}
		}

		spriteHandler->End();
		d3ddv->EndScene();
	}

	// Display back buffer content to the screen
	d3ddv->Present(NULL, NULL, NULL, NULL);
}

void CGame::RunGame()
{
	MSG msg;
	int done = 0;
	DWORD frameStart = GetTickCount64();
	DWORD tickPerFrame = 1000 / MAX_FRAME_RATE;

	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) done = 1;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		DWORD now = GetTickCount64();

		// dt: the time between (beginning of last frame) and now
		// this frame: the frame we are about to render
		this->dt = now - frameStart;

		if (this->dt >= tickPerFrame)
		{
			frameStart = now;

			g_inputHandler->ProcessKeyboard();

			this->UpdateGame(this->dt);
			this->RenderGame();
			this->ClearDeletedObject();
		}
		else
			Sleep(tickPerFrame - this->dt);
	}
}

CGame::~CGame()
{
	if (spriteHandler != NULL) {
		this->spriteHandler->Release();
	}

	if (backBuffer != NULL) {
		this->backBuffer->Release();
	}

	if (d3ddv != NULL) {
		this->d3ddv->Release();
	}

	if (d3d != NULL) {
		this->d3d->Release();
	}
}

CGame* CGame::GetInstance()
{
	if (__instance == NULL) __instance = new CGame();
	return __instance;
}

#pragma endregion

#pragma region Load Resources

void CGame::LoadResource()
{
	LPCWSTR sceneFilePath = SCENE2_PATH;
	DebugOut(L"[INFO] Start loading scene resouce %s\n", sceneFilePath);

	// start game camera
	camera->SetSize(Vector2D(this->mapWidth, this->mapHeight));

	// open scene file path
	ifstream fs;
	fs.open(sceneFilePath);

	SceneSection section = SceneSection::SCENE_SECTION_UNKNOW;
	char str[2048];
	while (fs.getline(str, 2048))
	{
		std::string line(str);

		// set section
		if (line[0] == '#') continue; // skip comment line
		if (line == "[TEXTURES]") {
			section = SceneSection::SCENE_SECTION_TEXTURES;
			continue;
		}
		if (line == "[SPRITES]") {
			section = SceneSection::SCENE_SECTION_SPRITES;
			continue;
		}
		if (line == "[ANIMATIONS]") {
			section = SceneSection::SCENE_SECTION_ANIMATIONS;
			continue;
		}
		if (line == "[MAP]") {
			section = SceneSection::SCENE_SECTION_MAP;
			continue;
		}
		if (line == "[PLATFORMS]") {
			section = SceneSection::SCENE_SECTION_PLATFORMS;
			continue;
		}
		if (line == "[OBJECTS]") {
			section = SceneSection::SCENE_SECTION_OBJECTS;
			continue;
		}
		if (line[0] == '[') {
			section = SceneSection::SCENE_SECTION_UNKNOW;
			continue;
		}

		switch (section)
		{
		case SceneSection::SCENE_SECTION_UNKNOW:
			break;
		case SceneSection::SCENE_SECTION_TEXTURES:
			this->__ParseSection_TEXTURES__(line);
			break;
		case SceneSection::SCENE_SECTION_SPRITES:
			this->__ParseSection_SPRITES__(line);
			break;
		case SceneSection::SCENE_SECTION_ANIMATIONS:
			this->__ParseSection_ANIMATIONS__(line);
			break;
		case SceneSection::SCENE_SECTION_MAP:
			this->__ParseSection_MAP__(line);
			break;
		case SceneSection::SCENE_SECTION_PLATFORMS:
			this->__ParseSection_PLATFORMS__(line);
			break;
		case SceneSection::SCENE_SECTION_OBJECTS:
			this->__ParseSection_OBJECTS__(line);
			break;	
		default:
			break;
		}
	}
	fs.close();
	DebugOut(L"[INFO] Load scene resource done\n");
}

void CGame::__ParseSection_TEXTURES__(std::string line)
{
	std::vector<std::string> tokens = SplitLine(line);
	if (tokens.size() < 5)
		return;

	int texId = atoi(tokens[0].c_str());
	wstring texPath = ToWSTR(tokens[1]);
	int R = atoi(tokens[2].c_str());
	int G = atoi(tokens[3].c_str());
	int B = atoi(tokens[4].c_str());

	g_textures->Add(texId, texPath.c_str(), D3DCOLOR_XRGB(R, G, B));
}

void CGame::__ParseSection_SPRITES__(std::string line)
{
	std::vector<std::string> tokens = SplitLine(line);
	if (tokens.size() < 6)
		return; // skip

	int id = atoi(tokens[0].c_str());
	int left = atoi(tokens[1].c_str());
	int top = atoi(tokens[2].c_str());
	int width = atoi(tokens[3].c_str());
	int height = atoi(tokens[4].c_str());
	int texId = atoi(tokens[5].c_str());

	LPDIRECT3DTEXTURE9 texOfThis = g_textures->Get(texId);
	if (texOfThis == nullptr) {
		DebugOut(L"[ERROR] TexId %d is not existed\n", texId);
		return; // skip
	}

	g_sprites->Add(id, left, top, width, height, texOfThis);
}

void CGame::__ParseSection_ANIMATIONS__(std::string line)
{
	std::vector<std::string> tokens = SplitLine(line);
	if (tokens.size() < 3)
		return; // skip
	LPANIMATION lpAni = new CAnimation();
	int id = atoi(tokens[0].c_str());
	for (int i = 1; i < tokens.size(); i += 2) {
		int spriteId = atoi(tokens[i].c_str());
		int frameTime = atoi(tokens[i + 1].c_str());
		lpAni->Add(spriteId, frameTime);
	}

	g_animations->Add(id, lpAni);
}

void CGame::__ParseSection_MAP__(std::string line)
{
	std::vector<std::string> tokens = SplitLine(line);
	if (tokens.size() < 2) {
		return;
	}

	float sectionWidth = atoi(tokens[1].c_str()) * TILESET_WIDTH;
	float sectionHeight = atoi(tokens[2].c_str()) * TILESET_HEIGHT;

	SRect mapBoundary = SRect(
		0,
		sectionHeight,
		sectionWidth,
		0
	);

	this->mapWidth = sectionWidth;
	this->mapHeight = sectionHeight;
	camera->SetBoundary(mapBoundary);
	quadtree = new CQuadTree(0, mapBoundary);

	std::string mapPath = tokens[0];
	int texId = 10; // 10 is map texId
	g_textures->Add(texId, ToWSTR(mapPath).c_str(), D3DCOLOR_XRGB(0, 0, 0));
	LPDIRECT3DTEXTURE9 texMap = g_textures->Get(texId);

	int sprMap = 3000; // 300 is map sprite id
	g_sprites->Add(sprMap, 0, 0, this->mapWidth, this->mapHeight, texMap);
	this->map = g_sprites->Get(sprMap);
}

void CGame::__ParseSection_PLATFORMS__(std::string line)
{
	std::vector<std::string> tokens = SplitLine(line);
	if (tokens.size() < 4) {
		return; // skip
	}

	float x = atoi(tokens[0].c_str()) * TILESET_WIDTH;
	float y = (atoi(tokens[1].c_str()) + 1) * TILESET_HEIGHT;
	float width = atoi(tokens[2].c_str()) * TILESET_WIDTH;
	float height = atoi(tokens[3].c_str()) * TILESET_HEIGHT;
	LPGAMEOBJECT platformObject = new CBrick(Vector2D(width, height));
	platformObject->SetPosition(Vector2D(x + width / 2, this->mapHeight - y + height / 2));
	this->NewGameObject(platformObject);
}

void CGame::__ParseSection_OBJECTS__(std::string line)
{
	std::vector<std::string> tokens = SplitLine(line);
	if (tokens.size() < 5) {
		return;
	}

	LPGAMEOBJECT newObject = nullptr;
	std::string objectType = tokens[0].c_str();

	if (objectType == "jason") {
		newObject = new CJason;
		jason = (CJason*)newObject;
	}
	else if (objectType == "sophia") {
		newObject = new CSophia;
		sophia = (CSophia*)newObject;
	}
	else if (objectType == "eyelet") newObject = new CEnemyEyelet;
	else if (objectType == "ballot") newObject = new CEnemyBallot;
	else {
		DebugOut(L"[ERROR] Unknowed object type: %s\n", objectType); // catch undefined object
		return;
	}

	PrepareGameObject(newObject, tokens);
}

void CGame::PrepareGameObject(LPGAMEOBJECT& object, std::vector<std::string> tokens)
{
	float x = atoi(tokens[1].c_str());
	float y = atoi(tokens[2].c_str());
	float width = atoi(tokens[3].c_str());
	float height = atoi(tokens[4].c_str());
	object->SetPosition(Vector2D(x + width / 2, this->mapHeight - y + height / 2));
	this->NewGameObject(object);
}

void CGame::NewGameObject(LPGAMEOBJECT& newObject)
{
	worldObjects.push_back(newObject);
	quadtree->Insert(newObject);
}

std::vector<LPGAMEOBJECT> CGame::GetRenderedObjects()
{
	return renderedObjects;
}

void CGame::ClearDeletedObject()
{
	for (int i = 0; i < worldObjects.size(); i++) {
		auto object = worldObjects.at(i);
		if (object->IsLive() == false) {
			worldObjects.erase(std::next(worldObjects.begin() + i - 1));
			quadtree->RemoveEntityFromLeafNodes(object);
			continue;
		}
	}
}

#pragma endregion