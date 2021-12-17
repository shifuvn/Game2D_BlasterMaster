#include "EnemyInterrupt.h"
#include "Player.h"
#include "EnemyNeoWorm.h"

#define V_BOXSIZE			Vector2D(22.0f, 18.0f)

constexpr auto AnimationDefaultId = "df";
constexpr auto SpriteDefaultId = "df";
constexpr auto MaxNeoworms = 2;
constexpr auto DelayNeoworm = 700;
constexpr auto DetectedPlayerXAxis = 10;

CEnemyInterrupt::CEnemyInterrupt()
{
	this->AddAnimation(AnimationDefaultId, "aniEInterrupt");
	this->AddSprite(SpriteDefaultId, "sprEInterrupt00");

	this->hp = 30;
	this->damage = 10;	// TODO: adjust damage later

	//
	this->colliders.clear();
	CCollider2D* collider = new CCollider2D(this, true, false, VectorZero(), V_BOXSIZE);
	this->colliders.push_back(collider);
	this->SetColliders(this->colliders);
}

void CEnemyInterrupt::Update(DWORD dt)
{
	LPGAMEOBJECT player = CPlayer::GetInstance()->GetPlayer();
	if (abs(this->position.x - player->GetPosition().x) <= DetectedPlayerXAxis 
		&& this->position.y > player->GetPosition().y
		&& (GetTickCount64() - this->prevTimeNeoworm) > DelayNeoworm
		&& this->existNeoworm < MaxNeoworms) {
		LPGAMEOBJECT neoworm = new CEnemyNeoWorm();
		neoworm->SetNx(RandomDirect());
		neoworm->SetPosition(this->position);
		CGame::GetInstance()->NewGameObject(neoworm);

		this->prevTimeNeoworm = GetTickCount64();
		this->existNeoworm++;
	}
}

void CEnemyInterrupt::Render()
{
	D3DCOLOR color = this->GetRenderColor();
	LPGAMEOBJECT player = CPlayer::GetInstance()->GetPlayer();

	if (abs(this->position.x - player->GetPosition().x) <= DetectedPlayerXAxis && this->position.y > player->GetPosition().y) {
		this->animations.at(AnimationDefaultId)->Render(this->position, this->nx, color);
	}
	else {
		this->sprites.at(SpriteDefaultId)->Draw(this->position, this->nx, color);
	}
}

void CEnemyInterrupt::OnCollision(CCollider2D* self, LPCOLLISIONEVENT coEvent)
{
	if (dynamic_cast<CSophia*>(coEvent->object)) {
		coEvent->object->TakeDamage(this->damage);
		this->TakeDamage(coEvent->object->GetDamage());

		STriggerTag tag = STriggerTag(coEvent->object);
		coEvent->object->AddTriggerTag(this);
		this->AddTriggerTag(coEvent->object);
	}
}

void CEnemyInterrupt::OnTrigger(CCollider2D* self, LPCOLLISIONEVENT coEvent)
{
}