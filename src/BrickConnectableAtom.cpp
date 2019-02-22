/*
 *  This file is part of esynth.
 *
 *  esynth is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  esynth is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with esynth.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>


#include "BrickConnectableAtom.h"
#include "Brick.h"
#include "AtomT.h"


/**********************************************************************************/

BrickConnectableAtom::BrickConnectableAtom(const BrickConnectableAtom* const that)
{
    this->atomType = that->atomType;
    this->ownerFragment = that->ownerFragment;
    this->theAtom.maxConnect = that->theAtom.maxConnect;
    this->theAtom.connectionID = that->theAtom.connectionID;
    this->theAtom.numExternalConnections = that->theAtom.numExternalConnections;
    this->theAtom.numAllowConns = that->theAtom.numAllowConns;

    this->allowableTypes = new AtomT*[this->theAtom.numAllowConns];

    for (int a = 0; a < that->theAtom.numAllowConns; a++)
    {
        this->allowableTypes[a] = new AtomT(that->allowableTypes[a]);
    }
}

/**********************************************************************************/

BrickConnectableAtom::BrickConnectableAtom(const std::string& aType, const Brick* const owner,
                                           const std::vector<std::string>& connTypes)
{
    this->atomType = AtomT(aType);
    this->ownerFragment = (Molecule*)owner;
    this->theAtom.maxConnect = 1;
    this->theAtom.numExternalConnections = 0;
    this->theAtom.numAllowConns = connTypes.size();

    this->allowableTypes = new AtomT*[this->theAtom.numAllowConns];

    for (unsigned a = 0; a < connTypes.size(); a++)
    {
        this->allowableTypes[a] = new AtomT(connTypes[a]);
    }
}

/**********************************************************************************/

BrickConnectableAtom::~BrickConnectableAtom()
{
    for (int a = 0; a < this->theAtom.numAllowConns; a++)
    {
        delete this->allowableTypes[a];
    }

    delete[] this->allowableTypes;
}

/**********************************************************************************/

bool BrickConnectableAtom::CanConnectTo(const Atom& that) const
{
    if (that.IsSimple()) return false;

    //
    // Are there any allowable spots in the atoms to connect?
    //
    if(!this->SpaceToConnect()) return false;

    if(!that.SpaceToConnect()) return false;

    //
    // Does this atom allow the connection to that?
    //
    bool canConnect = false;
    for (int t = 0; t < this->theAtom.numAllowConns; t++)
    {
        if ((*this->allowableTypes[t]) == that.getAtomType())
        {
            canConnect = true;
            break;
        }
    }

    if (!canConnect) return false;

    if (that.CanConnectToAny()) return true; 

    if (that.IsLinkerAtom()) return false;

    //
    // Does that atom allow the connection to this?
    //
    const BrickConnectableAtom& rAtom = static_cast<const BrickConnectableAtom&>(that);
    for (int t = 0; t < rAtom.theAtom.numAllowConns; t++)
    {
        if ((*rAtom.allowableTypes[t]) == this->atomType)
        {
            return true;
        }
    }

    return false;
}

/****************************************************************************************/

std::string BrickConnectableAtom::toString() const
{
    std::ostringstream oss;

    oss << " Connections{ Max: " << theAtom.maxConnect;

    oss << " Allow: ";

    for (int a = 0; a < this->theAtom.numAllowConns; a++)
    {
        oss << allowableTypes[a]->toString() << " ";
    }

    oss << "  Conn Id: (" << theAtom.connectionID << ")";

    oss << " }";

    return oss.str();
}

/****************************************************************************************/

std::ostream& operator<< (std::ostream& os, BrickConnectableAtom& atom)
{
    os << atom.toString();

    return os;
}

/****************************************************************************************/
