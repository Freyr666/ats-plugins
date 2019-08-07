/* audiodata.h
 *
 * Copyright (C) 2016 freyr <sky_rider_93@mail.ru> 
 *
 * This file is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 3 of the 
 * License, or (at your option) any later version. 
 *
 * This file is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */
#ifndef AUDIODATA_H
#define AUDIODATA_H

#include <glib.h>
/*
typedef struct __Param       Param;
typedef struct __AudioParams AudioParams;

struct __Param {
        double min;
        double max;
        double avg;
};

struct __AudioParams {
        Param    shortt;
        Param    moment;
        guint    counter_shortt;
        guint    counter_moment;
};

void param_reset (AudioParams*);
void param_avg (AudioParams*);
void param_add_shortt (AudioParams*, double);
void param_add_moment (AudioParams*, double);
*/
#endif /* AUDIODATA_H */
