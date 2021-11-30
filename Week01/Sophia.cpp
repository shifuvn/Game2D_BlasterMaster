#include "Sophia.h"

CSophia::CSophia()
{
	this->directState = SophiaDirectState::Stay;
	this->actionState = SophiaActionState::Idle;

	this->UpdateColliders();

	this->leftWheel = new CSophiaWheel(this);
	this->rightWheel = new CSophiaWheel(this);
	this->body = new CSophiaBody(this);
	this->cabin = new CSophiaCabin(this);
	this->gun = new CSophiaGun(this);
	this->leftWheel->AddAnimation(C_A_DEFAULT_KEY, 0); // 0: left wheel
	this->rightWheel->AddAnimation(C_A_DEFAULT_KEY, 1); // 1: right wheel
}

void CSophia::Update(DWORD dt)
{
	//LinearMoveInGravity(this, dt);
	LinearMove(this, dt);
	this->UpdateColliders();

	int backbufferWidth = CGame::GetInstance()->GetMapWidth();

	if (this->IsSelected()) {
		this->ListenKeyEvent();
	}
}

void CSophia::ListenKeyEvent()
{
	#pragma region LISTEN KEYBOARD

	auto input = CInputHandler::GetInstance();
	// observe direct state
	if (input->IsKeyDown(DIK_RIGHT)) {
		this->directState = SophiaDirectState::RightMove;
	}
	else if (input->IsKeyDown(DIK_LEFT)) {
		this->directState = SophiaDirectState::LeftMove;
	}
	else {
		this->directState = SophiaDirectState::Stay;
	}

	this->SubcribeDirectState(this->directState);

	// observe self action
	if (input->IsKeyDown(DIK_UP)) {
		DWORD now = GetTickCount64();
		this->stateTime = now;
		if (this->actionState == SophiaActionState::Idle) {
			this->actionState = SophiaActionState::Tile45;
		}
		else if (now - this->prevStateTime > 200 && this->actionState == SophiaActionState::Tile45) {
			this->actionState = SophiaActionState::Up90;
		}
	}
	else {
		DWORD now = GetTickCount64();
		this->prevStateTime = now;
		if (this->actionState == SophiaActionState::Up90) {
			this->actionState = SophiaActionState::Tile45;
		}
		else if (now - this->stateTime > 100 && this->actionState == SophiaActionState::Tile45) {
			this->actionState = SophiaActionState::Idle;
		}
	}

	if (input->OnKeyDown(DIK_X)) {
		if (this->velocity.y <= 0) {
			this->velocity.y = PLAYER_JUMP_FORCE;
		}
	}

	#pragma endregion
}

void CSophia::UpdateColliders()
{
	this->colliders.clear();
	auto collider = new CCollider2D;
	collider->SetGameObject(this);

	switch (this->actionState)
	{
	case SophiaActionState::Idle:
		collider->SetOffset(SOPHIA_OFFSET_IDLE);
		collider->SetBoxSize(SOPHIA_BOX_IDLE);
		break;
	case SophiaActionState::Tile45:
		collider->SetOffset(Vector2D(-0.5f * this->nx, 10.5f));
		collider->SetBoxSize(SOPHIA_BOX_TILE45);
		break;
	case SophiaActionState::Up90:
		collider->SetOffset(Vector2D(-3.0f * this->nx, 12.0f));
		collider->SetBoxSize(SOPHIA_BOX_UP90);
		break;
	default:
		break;
	}

	collider->SetDynamic(true);
	this->colliders.push_back(collider);
	this->SetColliders(colliders);
}

void CSophia::Render()
{
	this->leftWheel->Render();
	this->rightWheel->Render();
	this->body->Render();
	this->cabin->Render();
	this->gun->Render();

	auto debugPosSprite = CSprites::GetInstance()->Get(2000);
	Vector2D debugPos = Vector2D(this->position.x - 16, this->position.y);
	debugPosSprite->Draw(debugPos, this->nx, 255);
}

void CSophia::SubcribeDirectState(SophiaDirectState directState)
{
	switch (directState)
	{
	case SophiaDirectState::Stay:
		this->velocity.x = 0;
		break;

	case SophiaDirectState::LeftMove:
		this->velocity.x = -PLAYER_MOVING_SPEED;
		this->nx = -1;

		break;
	case SophiaDirectState::RightMove:
		this->velocity.x = PLAYER_MOVING_SPEED;
		this->nx = 1;

		break;
	default:
		this->velocity.x = 0;
		break;
	}
	
}

void CSophia::SubcribeActionState(SophiaActionState actionState)
{
}

void CSophia::OnCollision(CCollider2D* self, LPCOLLISIONEVENT coEvent)
{
	if (dynamic_cast<CBrick*>(coEvent->object) 
		&& !this->ground && coEvent->ny == 1) {
		this->ground = true;
	}
}

void CSophia::OnTrigger(CCollider2D* self, LPCOLLISIONEVENT coEvent)
{
}
