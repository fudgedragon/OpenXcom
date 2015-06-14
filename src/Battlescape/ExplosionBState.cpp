/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ExplosionBState.h"
#include "BattlescapeState.h"
#include "Explosion.h"
#include "TileEngine.h"
#include "UnitDieBState.h"
#include "Map.h"
#include "Camera.h"
#include "../Engine/Game.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Sound.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/Armor.h"
#include "../Engine/RNG.h"

namespace OpenXcom
{

/**
 * Sets up an ExplosionBState.
 * @param parent Pointer to the BattleScape.
 * @param center Center position in voxelspace.
 * @param item Item involved in the explosion (eg grenade).
 * @param unit Unit involved in the explosion (eg unit throwing the grenade).
 * @param tile Tile the explosion is on.
 * @param lowerWeapon Whether the unit causing this explosion should now lower their weapon.
 * @param range Distance between weapon and target.
 */
ExplosionBState::ExplosionBState(BattlescapeGame *parent, Position center, BattleActionType action, BattleItem *item, BattleUnit *unit, Tile *tile, bool lowerWeapon, int range) : BattleState(parent), _unit(unit), _center(center), _item(item), _damageType(), _tile(tile), _power(0), _radius(6), _range(range), _areaOfEffect(false), _lowerWeapon(lowerWeapon), _pistolWhip(false), _hit(false)
{
	_action.type = action;
	_action.weapon = item;
	_action.actor = unit;
	_action.target = center.toTile();
}

/**
 * Deletes the ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{

}

/**
 * Set new value to reference if new value is not equal -1.
 * @param oldValue old value to change.
 * @param newValue new value to set, but only if is not equal -1.
 */
void ExplosionBState::optValue(int& oldValue, int newValue) const
{
	if (newValue != -1)
	{
		oldValue = newValue;
	}
}

/**
 * Initializes the explosion.
 * The animation and sound starts here.
 * If the animation is finished, the actual effect takes place.
 */
void ExplosionBState::init()
{
	BattleType type = BT_NONE;
	BattleActionType action = _action.type;
	bool miss = false;
	if (_item)
	{
		type = _item->getRules()->getBattleType();
		_power = _item->getRules()->getPower();
		_damageType = _item->getRules()->getDamageType();
		_radius = _item->getRules()->getExplosionRadius();

		_pistolWhip = (type != BT_MELEE && action == BA_HIT);
		if (_pistolWhip)
		{
			_power = _item->getRules()->getMeleePower();
			_damageType = _parent->getRuleset()->getDamageType(DT_MELEE);
			_radius = 0;
		}
		else
		{
			// since melee aliens don't use a conventional weapon type, we use their strength instead.
			_power += _item->getRules()->getPowerBonus(_unit);
			_power -= _item->getRules()->getPowerRangeReduction(_range);
		}

		//testing if we hit target
		if (type == BT_PSIAMP)
		{
			if (action != BA_USE)
			{
				_power = 0;
			}
			if (!_parent->psiAttack(&_action))
			{
				_power = 0;
				miss = true;
			}
		}
		else if (type == BT_MELEE || _pistolWhip)
		{
			if (!_parent->getTileEngine()->meleeAttack(&_action))
			{
				_power = 0;
				miss = true;
			}
		}
		else if (type == BT_FIREARM)
		{
			if (_power <= 0)
			{
				miss = true;
			}
		}

		_areaOfEffect = type != BT_MELEE &&
						_item->getRules()->getExplosionRadius() != 0 &&
						(type != BT_PSIAMP || action == BA_USE) &&
						!_pistolWhip && !miss;
	}
	else if (_tile)
	{
		ItemDamageType DT;
		switch (_tile->getExplosiveType())
		{
		case 0:
			DT = DT_HE;
			break;
		case 5:
			DT = DT_IN;
			break;
		case 6:
			DT = DT_STUN;
			break;
		default:
			DT = DT_SMOKE;
			break;
		}
		_power = _tile->getExplosive();
		_tile->setExplosive(0, 0, true);
		_damageType = _parent->getRuleset()->getDamageType(DT);
		_radius = _power /10;
		_areaOfEffect = true;
	}
	else if (_unit && (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH || _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
	{
		RuleItem* corpse = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseGeoscape());
		_power = corpse->getPower();
		_power += corpse->getPowerBonus(_unit);
		_damageType = corpse->getDamageType();
		_radius = corpse->getExplosionRadius();
		_areaOfEffect = true;
		if (!RNG::percent(corpse->getSpecialChance()))
		{
			_power = 0;
		}
	}
	else
	{
		_power = 120;
		_damageType = _parent->getRuleset()->getDamageType(DT_HE);
		_areaOfEffect = true;
	}

	Tile *t = _parent->getSave()->getTile(_action.target);
	if (_areaOfEffect)
	{
		if (_power > 0)
		{
			int frame = ResourcePack::EXPLOSION_OFFSET;
			if (_item)
			{
				frame = _item->getRules()->getHitAnimation();
			}
			if (_parent->getDepth() > 0)
			{
				frame -= Explosion::EXPLODE_FRAMES;
			}
			int frameDelay = 0;
			int counter = std::max(1, (_power/5) / 5);
			_parent->getMap()->setBlastFlash(true);
			for (int i = 0; i < _power/5; i++)
			{
				int X = RNG::generate(-_power/2,_power/2);
				int Y = RNG::generate(-_power/2,_power/2);
				Position p = _center;
				p.x += X; p.y += Y;
				Explosion *explosion = new Explosion(p, frame, frameDelay, true);
				// add the explosion on the map
				_parent->getMap()->getExplosions()->push_back(explosion);
				if (i > 0 && i % counter == 0)
				{
					frameDelay++;
				}
			}
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED/2);
			// explosion sound
			if (_power <= 80)
				_parent->getResourcePack()->getSoundByDepth(_parent->getDepth(), ResourcePack::SMALL_EXPLOSION)->play();
			else
				_parent->getResourcePack()->getSoundByDepth(_parent->getDepth(), ResourcePack::LARGE_EXPLOSION)->play();

			_parent->getMap()->getCamera()->centerOnPosition(t->getPosition(), false);
		}
		else
		{
			_parent->popState();
		}
	}
	else
	// create a bullet hit
	{
		_parent->setStateInterval(std::max(1, ((BattlescapeState::DEFAULT_ANIM_SPEED/2) - (10 * _item->getRules()->getExplosionSpeed()))));
		_hit = _pistolWhip || type == BT_MELEE;
		bool psi = type == BT_PSIAMP && action != BA_USE;
		int anim = -1;
		int sound = -1;

		// melee weapon with ammo
		BattleItem *ammo = !_pistolWhip && _hit ? _item->getAmmoItem() : 0;

		if (_hit || psi)
		{
			anim = _item->getRules()->getMeleeAnimation();
			if (psi)
			{
				// psi attack sound is based weapon hit sound
				sound = _item->getRules()->getHitSound();

				optValue(anim, _item->getRules()->getPsiAnimation());
				optValue(sound, _item->getRules()->getPsiSound());
			}
			else
			{
				sound = _item->getRules()->getMeleeSound();
				if (ammo)
				{
					optValue(anim, ammo->getRules()->getMeleeAnimation());
					optValue(sound, ammo->getRules()->getMeleeSound());
				}
			}
		}
		else
		{
			anim = _item->getRules()->getHitAnimation();
			sound = _item->getRules()->getHitSound();
		}

		if (miss)
		{
			if (_hit || psi)
			{
				optValue(anim, _item->getRules()->getMeleeMissAnimation());
				if (psi)
				{
					// psi attack sound is based weapon hit sound
					optValue(sound, _item->getRules()->getHitMissSound());

					optValue(anim, _item->getRules()->getPsiMissAnimation());
					optValue(sound, _item->getRules()->getPsiMissSound());
				}
				else
				{
					optValue(sound, _item->getRules()->getMeleeMissSound());
					if (ammo)
					{
						optValue(anim, ammo->getRules()->getMeleeMissAnimation());
						optValue(sound, ammo->getRules()->getMeleeMissSound());
					}
				}
			}
			else
			{
				optValue(anim, _item->getRules()->getHitMissAnimation());
				optValue(sound, _item->getRules()->getHitMissSound());
			}
		}

		Explosion *explosion = new Explosion(_center, anim, 0, false, (_hit || psi)); // Don't burn the tile
		_parent->getMap()->getExplosions()->push_back(explosion);
		_parent->getMap()->getCamera()->setViewLevel(_center.z / 24);

		BattleUnit *target = t->getUnit();
		if ((_hit || psi) && _parent->getSave()->getSide() == FACTION_HOSTILE && target && target->getFaction() == FACTION_PLAYER)
		{
			_parent->getMap()->getCamera()->centerOnPosition(t->getPosition(), false);
		}
		if (sound != -1)
		{
			// bullet hit sound
			_parent->getResourcePack()->getSoundByDepth(_parent->getDepth(), sound)->play(-1, _parent->getMap()->getSoundAngle(_action.target));
		}
	}
}

/**
 * Animates explosion sprites. If their animation is finished remove them from the list.
 * If the list is empty, this state is finished and the actual calculations take place.
 */
void ExplosionBState::think()
{
	if (!_parent->getMap()->getBlastFlash())
	{
		for (std::list<Explosion*>::iterator i = _parent->getMap()->getExplosions()->begin(); i != _parent->getMap()->getExplosions()->end();)
		{
			if (!(*i)->animate())
			{
				delete (*i);
				i = _parent->getMap()->getExplosions()->erase(i);
				if (_parent->getMap()->getExplosions()->empty())
				{
					explode();
					return;
				}
			}
			else
			{
				++i;
			}
		}
	}
}

/**
 * Explosions cannot be cancelled.
 */
void ExplosionBState::cancel()
{
}

/**
 * Calculates the effects of the explosion.
 */
void ExplosionBState::explode()
{
	bool terrainExplosion = false;
	SavedBattleGame *save = _parent->getSave();
	// last minute adjustment: determine if we actually
	if (_hit)
	{
		if (_unit && !_unit->isOut())
		{
			_unit->aim(false);
			_unit->setCache(0);
		}

		if (_power <= 0)
		{
			_parent->getMap()->cacheUnits();
			_parent->popState();
			return;
		}

		int sound = _item->getRules()->getMeleeHitSound();
		if (!_pistolWhip)
		{
			// melee weapon with ammo
			BattleItem *ammo = _item->getAmmoItem();
			if (ammo)
			{
				optValue(sound, ammo->getRules()->getMeleeHitSound());
			}
		}
		if (sound != -1)
		{
			_parent->getResourcePack()->getSoundByDepth(_parent->getDepth(), sound)->play(-1, _parent->getMap()->getSoundAngle(_action.target));
		}
	}
	// after the animation is done, the real explosion/hit takes place
	if (_item)
	{
		if (!_unit && _item->getPreviousOwner())
		{
			_unit = _item->getPreviousOwner();
		}
	}

	if (_areaOfEffect)
	{
		save->getTileEngine()->explode(_center, _power, _damageType, _radius, _unit);
	}
	else
	{
		BattleUnit *victim = save->getTileEngine()->hit(_center, _power, _damageType, _unit, _action.type != BA_HIT);
		// check if this unit turns others into zombies
		if (!_item->getRules()->getZombieUnit().empty()
			&& RNG::percent(_item->getRules()->getSpecialChance())
			&& victim
			&& victim->getArmor()->getZombiImmune() == false
			&& victim->getSpawnUnit().empty()
			&& victim->getOriginalFaction() != FACTION_HOSTILE)
		{
			// converts the victim to a zombie on death
			victim->setRespawn(true);
			victim->setSpawnUnit(_item->getRules()->getZombieUnit());
		}
	}

	if (_tile)
	{
		terrainExplosion = true;
	}
	if (!_tile && !_item)
	{
		terrainExplosion = true;
	}

	// now check for new casualties
	_parent->checkForCasualties(_item, _unit, false, terrainExplosion);

	// if this explosion was caused by a unit shooting, now it's the time to put the gun down
	if (_unit && !_unit->isOut() && _lowerWeapon)
	{
		_unit->aim(false);
		_unit->setCache(0);
	}
	_parent->getMap()->cacheUnits();
	_parent->popState();

	// check for terrain explosions
	Tile *t = save->getTileEngine()->checkForTerrainExplosions();
	if (t)
	{
		Position p = t->getPosition().toVexel();
		p += Position(8,8,0);
		_parent->statePushFront(new ExplosionBState(_parent, p, BA_NONE, 0, _unit, t));
	}

	if (_item && (_item->getRules()->getBattleType() == BT_GRENADE || _item->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
	{
		for (std::vector<BattleItem*>::iterator j = _parent->getSave()->getItems()->begin(); j != _parent->getSave()->getItems()->end(); ++j)
		{
			if (_item->getId() == (*j)->getId())
			{
				_parent->getSave()->removeItem(_item);
				break;
			}
		}
	}
}

}
