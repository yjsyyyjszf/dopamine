/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#include "dopamine/authentication/AuthenticatorNone.h"

#include <odil/AssociationParameters.h>

#include "dopamine/authentication/AuthenticatorBase.h"

namespace dopamine
{

namespace authentication
{

AuthenticatorNone
::AuthenticatorNone()
: AuthenticatorBase()
{
    // Nothing to do
}

AuthenticatorNone
::~AuthenticatorNone()
{
    // Nothing to do
}

bool
AuthenticatorNone
::operator()(odil::AssociationParameters const & parameters) const
{
    return (parameters.get_user_identity().type ==
        odil::AssociationParameters::UserIdentity::Type::None);
}

} // namespace authentication

} // namespace dopamine
