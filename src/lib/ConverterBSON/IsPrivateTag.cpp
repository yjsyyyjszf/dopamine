/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#include "IsPrivateTag.h"

namespace dopamine
{

namespace converterBSON
{

IsPrivateTag
::IsPrivateTag():
    Condition()
{
    // Nothing else
}

IsPrivateTag
::~IsPrivateTag()
{
    // Nothing to do
}

bool
IsPrivateTag
::operator()(odil::Tag const & tag, odil::Element const &) const
{
    return tag.is_private();
}

} // namespace converterBSON

} // namespace dopamine
