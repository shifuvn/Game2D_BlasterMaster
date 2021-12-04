#include "Jason.h"

CJason::CJason()
{	
	this->AddAnimation("stay", 30);
	this->AddAnimation("walk", 31);

	this->directState = new CJasonDirectionState(this);
	this->directState->SetState(JasonDirectState::STAY);

	this->SetVelocity(VectorZero());
	//this->active = false;
	this->ground = false;

	//
	this->colliders.clear();
	auto collider = new CCollider2D;
	collider->SetGameObject(this);
	collider->SetOffset(VectorZero());
	collider->SetBoxSize(V_JASON_BOX_SIZE);
	collider->SetDynamic(true);
	this->colliders.push_back(collider);
	this->SetColliders(colliders);
}

void CJason::Update(DWORD dt)
{
	InGravityAffect(this, dt);

	if (CControllerObject::GetInstance()->SelectId() == ControllerObjectID::JASON) {
		ListenKeyEvent();
	}
}

void CJason::Render()
{
	auto key = this->directState->MappingStateOfAnimation();
		
	this->animations.at(key)->
		Render(this->position, -this->nx, 255);
}

void CJason::UpdateColliders()
{

}

void CJason::ListenKeyEvent()
{
	auto input = CInputHandler::GetInstance();

	// map direct state
	if (input->IsKeyDown(DIK_RIGHT)) {
		this->directState->SetState(JasonDirectState::RIGHTWALK);
	}
	else if (input->IsKeyDown(DIK_LEFT)) {
		this->directState->SetState(JasonDirectState::LEFTWALK);
	}
	else {
		this->directState->SetState(JasonDirectState::STAY);
	}

	if (input->OnKeyDown(DIK_X) && this->ground) {
		this->ground = false;
		this->directState->SetState(JasonDirectState::JUMP);
	}

	if (input->OnKeyDown(SWITCH_CONTROLLER_KEYCODE)) {
		auto controller = CControllerObject::GetInstance();
		if (controller->GetSophia()->GetColliders().at(0)->GetBoundingBox()
			.Contain(this->colliders.at(0)->GetBoundingBox())) {
			controller->Select(ControllerObjectID::SOPHIA);
		}
	}

	// subcribe direct state
	this->SubcribeDirectionState(this->directState->GetState());
}

void CJason::SubcribeDirectionState(JasonDirectState directState)
{
	this->directState->SetState(directState);
	switch (directState)
	{
	case JasonDirectState::RIGHTWALK:
		this->SetVelocity(Vector2D(PLAYER_MOVING_SPEED, this->velocity.y));
		this->nx = 1;
		this->directState->MoveForward();
		break;

	case JasonDirectState::LEFTWALK:
		this->SetVelocity(Vector2D(-PLAYER_MOVING_SPEED, this->velocity.y));
		this->nx = -1;
		this->directState->MoveBackward();
		break;

	case JasonDirectState::STAY:
		this->SetVelocity(Vector2D(0, this->velocity.y));
		this->directState->Stay();
		break;

	case JasonDirectState::JUMP:
		this->SetVelocity(Vector2D(this->velocity.x, PLAYER_JUMP_FORCE));
		this->directState->Jump();
		break;
	}
}

void CJason::OnCollision(CCollider2D* self, LPCOLLISIONEVENT coEvent)
{
	if (dynamic_cast<CBrick*>(coEvent->object)) {
		if (!this->ground && coEvent->ny == 1) {
			this->ground = true;
		}
	}
}

void CJason::OnTrigger(CCollider2D* self, LPCOLLISIONEVENT coEvent)
{
}