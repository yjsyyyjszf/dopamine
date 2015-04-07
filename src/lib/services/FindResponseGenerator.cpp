/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#include "FindResponseGenerator.h"

namespace dopamine
{

namespace services
{

FindResponseGenerator
::FindResponseGenerator(const std::string &username):
    QueryRetrieveGenerator(username, Service_Query) // base class initialisation
{
    // Nothing to do
}

FindResponseGenerator
::~FindResponseGenerator()
{
    // Nothing to do
}

} // namespace services

} // namespace dopamine
