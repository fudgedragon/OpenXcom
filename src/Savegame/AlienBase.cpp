/*
 * Copyright 2010-2014 OpenXcom Developers.
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
#include "AlienBase.h"
#include <sstream>
#include <time.h>
#include "../Engine/Options.h"
#include "../Engine/Language.h"

namespace OpenXcom
{

/**
 * Initializes an alien base
 */
AlienBase::AlienBase() : Target(), _id(0), _inBattlescape(false), _discovered(false)
{
	_baseSeed = time(0), _supplyLevel = 0, _crewLevel = 0; //Initialise my improved bases values - FD
}

/**
 *
 */
AlienBase::~AlienBase()
{
}

/**
 * Loads the alien base from a YAML file.
 * @param node YAML node.
 */
void AlienBase::load(const YAML::Node &node)
{
	Target::load(node);
	_id = node["id"].as<int>(_id);
	_race = node["race"].as<std::string>(_race);
	_inBattlescape = node["inBattlescape"].as<bool>(_inBattlescape);
	_discovered = node["discovered"].as<bool>(_discovered);
	_baseSeed = node["seed"].as<uint64_t>(_baseSeed);
	_supplyLevel = node["supply"].as<int>(_supplyLevel);
	_crewLevel = node["crew"].as<int>(_crewLevel);
}

/**
 * Saves the alien base to a YAML file.
 * @return YAML node.
 */
YAML::Node AlienBase::save() const
{
	YAML::Node node = Target::save();
	node["id"] = _id;
	node["race"] = _race;
	if (_inBattlescape)
		node["inBattlescape"] = _inBattlescape;
	if (_discovered)
		node["discovered"] = _discovered;
	if (Options::improvedAlienBases)
	{
		node["seed"] = _baseSeed;
		node["supply"] = _supplyLevel;
		node["crew"] = _crewLevel;
	}
	return node;
}

/**
 * Saves the alien base's unique identifiers to a YAML file.
 * @return YAML node.
 */
YAML::Node AlienBase::saveId() const
{
	YAML::Node node = Target::saveId();
	node["type"] = "STR_ALIEN_BASE";
	node["id"] = _id;
	return node;
}

/**
 * Returns the alien base's unique ID.
 * @return Unique ID.
 */
int AlienBase::getId() const
{
	return _id;
}

/**
 * Changes the alien base's unique ID.
 * @param id Unique ID.
 */
void AlienBase::setId(int id)
{
	_id = id;
}

/**
 * Returns the alien base's unique identifying name.
 * @param lang Language to get strings from.
 * @return Full name.
 */
std::wstring AlienBase::getName(Language *lang) const
{
	return lang->getString("STR_ALIEN_BASE_").arg(_id);
}

/**
 * Returns the alien race currently residing in the alien base.
 * @return Alien race.
 */
std::string AlienBase::getAlienRace() const
{
	return _race;
}

/**
 * Changes the alien race currently residing in the alien base.
 * @param race Alien race.
 */
void AlienBase::setAlienRace(const std::string &race)
{
	_race = race;
}

/**
 * Gets the alien base's battlescape status.
 * @return Is the base on the battlescape?
 */
bool AlienBase::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Sets the alien base's battlescape status.
 * @param inbattle True if it's in battle, False otherwise.
 */
void AlienBase::setInBattlescape(bool inbattle)
{
	_inBattlescape = inbattle;
}

/**
 * Gets the alien base's geoscape status.
 * @return Has the base been discovered?
 */
bool AlienBase::isDiscovered() const
{
	return _discovered;
}

/**
 * Sets the alien base's geoscape status.
 * @param discovered Has the base been discovered?
 */
void AlienBase::setDiscovered(bool discovered)
{
	_discovered = discovered;
}

/**
 * Gets the seed used for generating this base's battlescape maps.
 * @return seed current base uses for battlescape generation.
 */
uint64_t AlienBase::getSeed() const
{
	return _baseSeed;
}

/**
 * Gets the base supply level.
 * @return percentage of current base supply.
 */
int AlienBase::getSupply() const
{
	return _supplyLevel;
}

/**
 * Sets the base supply level.
 * @param supply percentage of current base supply.
 */
void AlienBase::setSupply(int supply)
{
	_supplyLevel = supply;
}

/**
 * Gets the base crew level.
 * @return percentage of current base crew.
 */
int AlienBase::getCrew() const
{
	return _crewLevel;
}

/**
 * Sets the base crew level.
 * @param crew percentage of current base crew.
 */
void AlienBase::setCrew(int crew)
{
	_crewLevel = crew;
}

}
