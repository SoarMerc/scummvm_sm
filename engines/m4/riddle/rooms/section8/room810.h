/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef M4_RIDDLE_ROOMS_SECTION8_ROOM810_H
#define M4_RIDDLE_ROOMS_SECTION8_ROOM810_H

#include "m4/riddle/rooms/room.h"

namespace M4 {
namespace Riddle {
namespace Rooms {

class Room810 : public Room {
public:
	Room810() : Room() {}
	~Room810() override {}

	void preload() override;
	void init() override;
	void pre_parser() override;
	void parser() override;
	void daemon() override;

private:
	bool _alreadyPlayedVideo04aFl = false;

	int32 _810BlockSlidesOutSeries = 0;
	int32 _810FireFlickerSeries = 0;
	int32 _810LitUrnSeries = 0;
	int32 _810MercSeries = 0;
	int32 _ripleyTakesJadeSealFromTombSeries = 0;
	int32 _ripPos3LookAroundSeries = 0;
	int32 _ripTrekHandTalkPos3Series = 0;

	machine *_810MercMach = nullptr;
	machine *_810SealMach = nullptr;
	machine *_blockSlidesOutMach = nullptr;
	machine *_ripLooksAroundAndNodsMach = nullptr;
	machine *_safariShadow3Mach = nullptr;
};

} // namespace Rooms
} // namespace Riddle
} // namespace M4

#endif
