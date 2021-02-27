/**
This file is part of pax-LampD1 (https://github.com/CalinRadoni/pax-LampD1)
Copyright (C) 2019 by Calin Radoni

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HTTPSrv_H
#define HTTPSrv_H

#include "pax_http_server.h"

class HTTPSrv : public PaxHttpServer
{
public:
    HTTPSrv(void);
    virtual ~HTTPSrv();

    /**
     * Data for status string
     */
    uint32_t animationID = 0;
    uint32_t currentColor = 0x010101;
    uint32_t currentIntensity = 0;

protected:
    /**
     * @warning Delete returned string with 'free' !
     */
    virtual char* CreateJSONStatusString(bool addWhitespaces);
};

#endif
