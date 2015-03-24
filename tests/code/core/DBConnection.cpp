/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#define BOOST_TEST_MODULE ModuleDBConnection
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/test/unit_test.hpp>

#include "core/ConfigurationPACS.h"
#include "core/DBConnection.h"
#include "core/ExceptionPACS.h"

/*************************** TEST OK 01 *******************************/
/**
 * Nominal test case: Constructor/Destructor
 */
struct TestDataOK01
{
    TestDataOK01()
    {
        std::string NetworkConfFILE(getenv("DOPAMINE_TEST_CONFIG"));
        dopamine::ConfigurationPACS::get_instance().Parse(NetworkConfFILE);
    }

    ~TestDataOK01()
    {
        dopamine::ConfigurationPACS::delete_instance();
    }
};

BOOST_FIXTURE_TEST_CASE(Constructor, TestDataOK01)
{
    dopamine::DBConnection connection;

    BOOST_CHECK_EQUAL(connection.get_connection().isFailed(), false);
}

/*************************** TEST OK 02 *******************************/
/**
 * Nominal test case: checkUserAuthorization
 */
BOOST_FIXTURE_TEST_CASE(Check_Authorization, TestDataOK01)
{
    dopamine::DBConnection connection;

    UserIdentityNegotiationSubItemRQ * identity = new UserIdentityNegotiationSubItemRQ();
    identity->setIdentityType(ASC_USER_IDENTITY_USER_PASSWORD);
    identity->setPrimField("unknown", 7);
    identity->setSecField("password2", 9);

    BOOST_CHECK_EQUAL(connection.checkUserAuthorization(*identity, DIMSE_C_ECHO_RQ),
                      false);

    delete identity;
}
