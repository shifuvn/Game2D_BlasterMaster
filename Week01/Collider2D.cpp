#include "Collider2D.h"
#include "Textures.h"

#include "Jason.h"
#include "Sophia.h"

#define MARGIN		0.2f

void CCollider2D::SweptAABB(SRect movingRect, SRect staticRect, 
	float dx, float dy, float& nx, float& ny, float& dt)
{
	float dxEntry, dxExit, txEntry, txExit;
	float dyEntry, dyExit, tyEntry, tyExit;

	float tEntry, tExit;

	dt = -1.0f;			// no collision
	nx = ny = 0.0f;

	//
	// Broad-phase test 
	//

	SRect rBroadPhase;
	rBroadPhase.left = dx > 0 ? movingRect.left : movingRect.left + dx;
	rBroadPhase.top = dy > 0 ? movingRect.top + dy : movingRect.top;
	rBroadPhase.right = dx > 0 ? movingRect.right + dx : movingRect.right;
	rBroadPhase.bottom = dy > 0 ? movingRect.bottom : movingRect.bottom + dy;

	if (!rBroadPhase.Overlap(staticRect)) return;

	if (dx == 0 && dy == 0) return;		// moving object is not moving > obvious no collision

	if (dx > 0)
	{
		dxEntry = staticRect.left - movingRect.right;
		dxExit = staticRect.right - movingRect.left;
	}
	else if (dx < 0)
	{
		dxEntry = staticRect.right - movingRect.left;
		dxExit = staticRect.left - movingRect.right;
	}

	if (dy > 0)
	{
		dyEntry = staticRect.bottom - movingRect.top;
		dyExit = staticRect.top - movingRect.bottom;
	}
	else if (dy < 0)
	{
		dyEntry = staticRect.top - movingRect.bottom;
		dyExit = staticRect.bottom - movingRect.top;
	}

	if (dx == 0)
	{
		txEntry = -VectorInfinity().x;
		txExit = VectorInfinity().x;
	}
	else
	{
		txEntry = dxEntry / dx;
		txExit = dxExit / dx;
	}

	if (dy == 0)
	{
		tyEntry = -VectorInfinity().y;
		tyExit = VectorInfinity().y;
	}
	else
	{
		tyEntry = dyEntry / dy;
		tyExit = dyExit / dy;
	}

	if ((txEntry < 0.0f && tyEntry < 0.0f)
		|| txEntry > 1.0f || tyEntry > 1.0f) {
		return;
	}

	tEntry = max(txEntry, tyEntry);
	tExit = min(txExit, tyExit);

	if (tEntry > tExit)
		return; // end

	dt = tEntry;

	if (txEntry > tyEntry)
	{
		ny = 0.0f;
		dx > 0 ? nx = -1.0f : nx = 1.0f;
	}
	else
	{
		nx = 0.0f;
		dy > 0 ? ny = -1.0f : ny = 1.0f;
	}
}

LPCOLLISIONEVENT CCollider2D::SweptAABBEx(CCollider2D* coOther)
{
	SRect staticRect;
	SRect movingRect;
	float t, nx, ny;

	staticRect = coOther->GetBoundingBox();

	auto coObject = coOther->GetGameObject();
	float sdx = coObject->GetVelocity().x * CGame::GetDeltaTime();
	float sdy = coObject->GetVelocity().y * CGame::GetDeltaTime();

	float rdx = this->dx - sdx;
	float rdy = this->dy - sdy;

	movingRect = GetBoundingBox();

	SweptAABB(
		movingRect, staticRect,
		rdx, rdy, nx, ny, t);

	SCollisionEvent* e = new SCollisionEvent(t, nx, ny, coObject, coOther);
	return e;
}

void CCollider2D::PredictPotentialCollision(std::vector<LPGAMEOBJECT>* coObjects, 
	std::vector<LPCOLLISIONEVENT>& coEvents)
{
	for (UINT i = 0; i < coObjects->size(); i++)
	{
		auto coObject = coObjects->at(i);

		if (this->object == coObject)
			continue;

		if (coObject->IsLive() == false)
			continue;

		if (coObject->IsActive() == false)
			continue;

		for (auto co : coObject->GetColliders())
		{
			LPCOLLISIONEVENT e = SweptAABBEx(co);

			if (e->t > 0 && e->t <= 1.0f) {
				coEvents.push_back(e);
			}
			else {
				delete e;
			}
		}
	}

	std::sort(coEvents.begin(), coEvents.end(), SCollisionEvent::compare);
}

void CCollider2D::FilterCollision(std::vector<LPCOLLISIONEVENT>& coEvents, 
	LPCOLLISIONEVENT& colX, LPCOLLISIONEVENT& colY, 
	bool filterX = true, bool filterY = true)
{
	float minTx, minTy;

	minTx = 1.0f;
	minTy = 1.0f;
	int minIX = -1;
	int minIY = -1;

	int coEventsSize = coEvents.size();

	for (UINT i = 0; i < coEventsSize; i++)
	{
		LPCOLLISIONEVENT collisionEvent = coEvents.at(i);
		
		if (collisionEvent->deleted || collisionEvent->collider->IsTrigger())
			continue;

		if (collisionEvent->t < minTx && collisionEvent->nx != 0 && filterX) {
			minTx = collisionEvent->t; 
			minIX = i;
		}

		if (collisionEvent->t < minTy && collisionEvent->ny != 0 && filterY) {
			minTy = collisionEvent->t; 
			minIY = i;
		}
	}

	if (minIX >= 0) {
		colX = coEvents[minIX];
	}
	if (minIY >= 0) {
		colY = coEvents[minIY];
	}
}

void CCollider2D::PhysicalUpdate(std::vector<LPGAMEOBJECT>* coObjects)
{
	// check null or not dynamic
	if (this->object == nullptr || this->dynamic == false || this->object->IsLive() == false) return;

	// get delta time
	auto dt = CGame::GetDeltaTime();
	// get object position, velocity
	auto position = this->object->GetPosition();
	auto velocity = this->object->GetVelocity();

	this->dx = velocity.x * dt;
	this->dy = velocity.y * dt;

	this->coEvents.clear();
	this->coEventX = nullptr;
	this->coEventY = nullptr;

	PredictPotentialCollision(coObjects, this->coEvents);

	if (this->coEvents.size() == 0)
	{
		position.x += dx;
		position.y += dy;
		this->object->SetPosition(position);
	}
	else
	{
		FilterCollision(this->coEvents, this->coEventX, this->coEventY);

		if (this->coEventX != nullptr && this->coEventY != nullptr)
		{
			// collision y first
			if (this->coEventY->t < this->coEventX->t)
			{
				if (!this->trigger)
				{
					position.y += this->coEventY->t * dy + this->coEventY->ny * MARGIN;
					this->object->SetPosition(position);

					if (this->coEventY->ny != 0) {
						velocity.y = 0;
					}
					this->object->SetVelocity(velocity);
				}
				else {
					position.y += dy;
				}

				this->object->SetPosition(position);

				if (this->coEventY->ny != 0) {
					if (this->trigger == false) {
						this->object->OnCollision(this, this->coEventY);
					}
					else {
						this->object->OnTrigger(this, this->coEventY);
					}
				}

				//
				// see if after correction on Y, is there still a collision on X ? 
				//
				LPCOLLISIONEVENT colX_other = nullptr;

				this->coEventX->deleted = true;

				// replace with a new collision event using corrected location 
				this->coEvents.push_back(SweptAABBEx(this->coEventX->collider));

				// re-filter on X only
				FilterCollision(this->coEvents, colX_other, this->coEventY, true, false);

				if (colX_other != nullptr)
				{
					if (this->trigger == false)
						position.x += colX_other->t * dx + colX_other->nx * MARGIN;

					if (this->trigger == false) {
						this->object->OnCollision(this, colX_other);
					}
					else {
						this->object->OnTrigger(this, colX_other);
					}
				}
				else
				{
					position.x += dx;
				}
			}
			// collision on X first
			else
			{
				if (this->trigger == false)
				{
					position.x += this->coEventX->t * dx + this->coEventX->nx * MARGIN;
					this->object->SetPosition(position);

					if (this->coEventX->nx != 0) {
						velocity.x = 0;
					}
					this->object->SetVelocity(velocity);
				}

				if (this->coEventX->nx != 0) {
					if (this->trigger == false) {
						this->object->OnCollision(this, coEventX);
					}
					else {
						this->object->OnTrigger(this, this->coEventX);
					}
				}

				//
				// see if after correction on X, is there still a collision on Y ? 
				//
				LPCOLLISIONEVENT colY_other = nullptr;

				this->coEventY->deleted = true;

				// replace with a new collision event using corrected location 
				this->coEvents.push_back(SweptAABBEx(this->coEventY->collider));

				// re-filter on X only
				FilterCollision(this->coEvents, this->coEventX, colY_other, false, true);

				if (colY_other != nullptr)
				{
					if (this->trigger == false) {
						position.y += colY_other->t * dy + colY_other->ny * MARGIN;
					}

					if (this->trigger == false) {
						this->object->OnCollision(this, colY_other);
					}
					else {
						this->object->OnTrigger(this, colY_other);
					}
				}
				else
				{
					position.y += dy;
				}
			}
		}
		else
		{
			if (this->coEventX != nullptr)
			{
				if (this->trigger == false)
				{
					position.x += this->coEventX->t * dx + this->coEventX->nx * MARGIN;
					position.y += dy;
					this->object->SetPosition(position);

					if (this->coEventX->nx != 0) {
						velocity.x = 0;
					}
					this->object->SetVelocity(velocity);

					this->object->OnCollision(this, this->coEventX);
				}
				else {
					position.x += dx;
					position.y += dy;
					this->object->OnTrigger(this, this->coEventX);
				}
			}
			else
			{
				if (this->coEventY != nullptr)
				{
					if (this->trigger == false)
					{
						position.x += dx;
						position.y += this->coEventY->t * dy + this->coEventY->ny * MARGIN;
						this->object->SetPosition(position);

						if (this->coEventY->ny != 0) {
							velocity.y = 0;
						}
						this->object->SetVelocity(velocity);
						
						this->object->OnCollision(this, this->coEventY);
					}
					else {
						position.x += dx;
						position.y += dy;
						this->object->OnTrigger(this, this->coEventY);
					}
				}
				// both colX & colY are NULL 
				else
				{
					position.x += dx;
					position.y += dy;
				}
			}
		}
	}
	this->object->SetPosition(position);

	for (UINT i = 0; i < this->coEvents.size(); i++) {
		if (this->coEvents.at(i)->collider->IsTrigger()) {
			if (this->trigger == false) {
				this->object->OnCollision(this, this->coEvents.at(i));
			}
			else {
				this->object->OnTrigger(this, this->coEvents.at(i));
			}
		}
	}

	for (UINT i = 0; i < this->coEvents.size(); i++) {
		delete this->coEvents.at(i);
	}
}

SRect CCollider2D::GetBoundingBox()
{
	Vector2D posCollider = object->GetPosition() + offset;
	SRect boundingBox;
	boundingBox.left = posCollider.x - boxSize.x / 2;
	boundingBox.top = posCollider.y + boxSize.y / 2;
	boundingBox.right = posCollider.x + boxSize.x / 2;
	boundingBox.bottom = posCollider.y - boxSize.y / 2;
	return boundingBox;
}

void CCollider2D::RenderBoundingBox()
{
	Vector2D positionCollider = this->object->GetPosition() + this->offset;
	LPDIRECT3DTEXTURE9 bbox = CTextures::GetInstance()->Get(20); // texId 20: bbox for render collider
	SRect rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = this->boxSize.x;
	rect.bottom = this->boxSize.y;

	CGame::GetInstance()->Draw(positionCollider, -1, bbox, rect.left, rect.top, rect.right, rect.bottom, 50);
}