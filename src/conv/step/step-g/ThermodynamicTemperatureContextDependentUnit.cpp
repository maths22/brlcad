/*                 ThermodynamicTemperatureContextDependentUnit.cpp
 * BRL-CAD
 *
 * Copyright (c) 1994-2016 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file step/ThermodynamicTemperatureContextDependentUnit.cpp
 *
 * Routines to convert STEP "ThermodynamicTemperatureContextDependentUnit" to BRL-CAD BREP
 * structures.
 *
 */

#include "STEPWrapper.h"
#include "Factory.h"

#include "ThermodynamicTemperatureContextDependentUnit.h"

#define CLASSNAME "ThermodynamicTemperatureContextDependentUnit"
#define ENTITYNAME "Thermodynamic_Temperature_Context_Dependent_Unit"
string ThermodynamicTemperatureContextDependentUnit::entityname = Factory::RegisterClass(ENTITYNAME, (FactoryMethod)ThermodynamicTemperatureContextDependentUnit::Create);

ThermodynamicTemperatureContextDependentUnit::ThermodynamicTemperatureContextDependentUnit()
{
    step = NULL;
    id = 0;
}

ThermodynamicTemperatureContextDependentUnit::ThermodynamicTemperatureContextDependentUnit(STEPWrapper *sw, int step_id)
{
    step = sw;
    id = step_id;
}

ThermodynamicTemperatureContextDependentUnit::~ThermodynamicTemperatureContextDependentUnit()
{
}

bool
ThermodynamicTemperatureContextDependentUnit::Load(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    step = sw;
    id = sse->STEPfile_id;


    // load base class attributes
    if (!ThermodynamicTemperatureUnit::Load(step, sse)) {
	std::cout << CLASSNAME << ":Error loading base class ::Unit." << std::endl;
	sw->entity_status[id] = STEP_LOAD_ERROR;
	return false;
    }
    if (!ContextDependentUnit::Load(step, sse)) {
	std::cout << CLASSNAME << ":Error loading base class ::Unit." << std::endl;
	sw->entity_status[id] = STEP_LOAD_ERROR;
	return false;
    }

    sw->entity_status[id] = STEP_LOADED;

    return true;
}

void
ThermodynamicTemperatureContextDependentUnit::Print(int level)
{
    TAB(level);
    std::cout << CLASSNAME << ":" << "(";
    std::cout << "ID:" << STEPid() << ")" << std::endl;

    TAB(level);
    std::cout << "Inherited Attributes:" << std::endl;
    ThermodynamicTemperatureUnit::Print(level + 1);
    ContextDependentUnit::Print(level + 1);

}

STEPEntity *
ThermodynamicTemperatureContextDependentUnit::GetInstance(STEPWrapper *sw, int id)
{
    return new ThermodynamicTemperatureContextDependentUnit(sw, id);
}

STEPEntity *
ThermodynamicTemperatureContextDependentUnit::Create(STEPWrapper *sw, SDAI_Application_instance *sse)
{
    return STEPEntity::CreateEntity(sw, sse, GetInstance, CLASSNAME);
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
