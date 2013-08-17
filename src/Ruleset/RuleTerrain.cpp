/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#include "RuleTerrain.h"
#include "MapBlock.h"
#include "MapDataSet.h"
#include "../Engine/RNG.h"
#include "Ruleset.h"

namespace OpenXcom
{

/**
* RuleTerrain construction
*/
RuleTerrain::RuleTerrain(const std::string &name) : _name(name), _largeBlockLimit(0), _hemisphere(0)
{
}

/**
* Ruleterrain only holds mapblocks. Map datafiles are referenced.
*/
RuleTerrain::~RuleTerrain()
{
	for (std::vector<MapBlock*>::iterator i = _mapBlocks.begin(); i != _mapBlocks.end(); ++i)
	{
		delete *i;
	}
}

/**
 * Loads the terrain from a YAML file.
 * @param node YAML node.
 * @param ruleset Ruleset for the terrain.
 */
void RuleTerrain::load(const YAML::Node &node, Ruleset *ruleset)
{
	if (const YAML::Node &map = node["mapDataSets"])
	{
		_mapDataSets.clear();
		for (YAML::const_iterator i = map.begin(); i != map.end(); ++i)
		{
			_mapDataSets.push_back(ruleset->getMapDataSet(i->as<std::string>()));
		}
	}
	if (const YAML::Node &map = node["mapBlocks"])
	{
		_mapBlocks.clear();
		for (YAML::const_iterator i = map.begin(); i != map.end(); ++i)
		{
			MapBlock *map = new MapBlock(this, (*i)["name"].as<std::string>(), 0, 0, MT_DEFAULT);
			map->load(*i);
			_mapBlocks.push_back(map);
		}
	}
	_name = node["name"].as<std::string>(_name);
	_largeBlockLimit = node["largeBlockLimit"].as<int>(_largeBlockLimit);
	_textures = node["textures"].as< std::vector<int> >(_textures);
	_hemisphere = node["hemisphere"].as<int>(_hemisphere);
}

/**
* gets a pointer to the array of mapblock
* @return pointer to the array of mapblocks
*/
std::vector<MapBlock*> *RuleTerrain::getMapBlocks()
{
	return &_mapBlocks;
}

/**
* gets a pointer to the array of mapdatafiles
* @return pointer to the array of mapdatafiles
*/
std::vector<MapDataSet*> *RuleTerrain::getMapDataSets()
{
	return &_mapDataSets;
}

/**
* gets the terrain name (string)
* @return name
*/
std::string RuleTerrain::getName() const
{
	return _name;
}

/**
* gets a random mapblock within the given constraints
* @param maxsize maximum size of the mapblock (10 or 20 or 999-don't care)
* @param type whether this must be a block of a certain type
* @param force whether to enforce the max size.
* @return pointer to mapblock
*/
MapBlock* RuleTerrain::getRandomMapBlock(int maxsize, MapBlockType type, bool force)
{
	std::vector<MapBlock*> compliantMapBlocks;

	for (std::vector<MapBlock*>::const_iterator i = _mapBlocks.begin(); i != _mapBlocks.end(); ++i)
	{
		if (((force && (*i)->getSizeX() == maxsize) || 
			(!force && (*i)->getSizeX() <= maxsize)) && 
			((*i)->getType() == type || (*i)->getSubType() == type))
		{
			for (int j = 0; j != (*i)->getRemainingUses(); ++j)
			{
				compliantMapBlocks.push_back((*i));
			}
		}
	}

	if (compliantMapBlocks.empty()) return 0;

	int n = RNG::generate(0, compliantMapBlocks.size() - 1);

	if (type == MT_DEFAULT)
		compliantMapBlocks[n]->markUsed();

	return compliantMapBlocks[n];
}

/**
* gets a mapblock with a given name
* @param name
* @return pointer to mapblock
*/
MapBlock* RuleTerrain::getMapBlock(const std::string &name)
{
	for (std::vector<MapBlock*>::const_iterator i = _mapBlocks.begin(); i != _mapBlocks.end(); ++i)
	{
		if((*i)->getName() == name)
			return (*i);
	}
	return 0;
}

/**
* Gets a mapdata object.
* @param id the id in the terrain
* @param mapDataSetID id to the map data set
* @return pointer to object
*/
MapData *RuleTerrain::getMapData(int *id, int *mapDataSetID) const
{
	MapDataSet* mdf = 0;

	for (std::vector<MapDataSet*>::const_iterator i = _mapDataSets.begin(); i != _mapDataSets.end(); ++i)
	{
		mdf = *i;
		if (*id - mdf->getSize() < 0)
		{
			break;
		}
		*id -= mdf->getSize();
		(*mapDataSetID)++;
	}

	return mdf->getObjects()->at(*id);
}
/**
* Gets the maximum amount of large blocks this terrain.
*/
int RuleTerrain::getLargeBlockLimit() const
{
	return _largeBlockLimit;
}
/**
* Resets the remaining uses of each mapblock.
*/
void RuleTerrain::resetMapBlocks()
{
	for (std::vector<MapBlock*>::const_iterator i = _mapBlocks.begin(); i != _mapBlocks.end(); ++i)
	{
		(*i)->reset();
	}
}

/**
* gets a pointer to the array of globe texture IDs this terrain is loaded on
* @return pointer to the array of texture IDs
*/
std::vector<int> *RuleTerrain::getTextures()
{
	return &_textures;
}
/**
* Gets the hemishpere this terrain occurs on.
* -1 = northern, 0 = either, 1 = southern
*/
int RuleTerrain::getHemisphere() const
{
	return _hemisphere;
}


}
