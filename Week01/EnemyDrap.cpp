#include "EnemyDrap.h"

#define AnimationDefaultId	"df"

CEnemyDrap::CEnemyDrap()
{
	this->AddAnimation(AnimationDefaultId, "aniEDrap"); // 200: drap animation id
}

void CEnemyDrap::Update(DWORD dt)
{
	this->position += this->velocity * dt;
}

void CEnemyDrap::Render()
{
	// get id of state

	// render
	auto animation = this->animations.at(AnimationDefaultId);
	animation->SetWait(false);
	animation->Render(this->position, 1, DrawArgbColorDefault());
}
