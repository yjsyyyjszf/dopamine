/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#define BOOST_TEST_MODULE ModuleWado_rs

#include <sstream>

#include <mimetic/mimeentity.h>

#include "core/ExceptionPACS.h"
#include "services/webservices/Wado_rs.h"
#include "services/webservices/WebServiceException.h"
#include "../ServicesTestClass.h"

/**
 * Pre-conditions:
 *     - we assume that ConfigurationPACS works correctly
 *
 *     - Following Environment variables should be defined
 *          * DOPAMINE_TEST_CONFIG
 *          * DOPAMINE_TEST_DICOMFILE
 */

void check_response(std::string const & response, std::string const & boundary)
{
    BOOST_REQUIRE(response != "");
    BOOST_REQUIRE_EQUAL(response.size(), 1735);
    BOOST_REQUIRE(response.find(SOP_INSTANCE_UID_01_01_01_01) !=
                  std::string::npos);

    // Parse MIME Message
    std::stringstream streamresponse;
    streamresponse << dopamine::services::MIME_VERSION << "\n"
                   << dopamine::services::CONTENT_TYPE
                   << dopamine::services::MIME_TYPE_MULTIPART_RELATED << "; "
                   << dopamine::services::ATTRIBUT_BOUNDARY
                   << boundary << "\n" << "\n";
    streamresponse << response << "\n";
    mimetic::MimeEntity entity(streamresponse);

    // Check Header
    mimetic::Header& h = entity.header();
    BOOST_CHECK(h.contentType().isMultipart());
    std::string content_type = h.contentType().str();
    BOOST_CHECK(content_type.find(dopamine::services::MIME_TYPE_MULTIPART_RELATED) !=
                std::string::npos);

    // Check each parts
    mimetic::MimeEntityList& parts = entity.body().parts();
    for(mimetic::MimeEntityList::iterator mbit = parts.begin();
        mbit != parts.end(); ++mbit)
    {
        // check Header
        mimetic::Header& header = (*mbit)->header();
        std::stringstream contenttypestream;
        contenttypestream << header.contentType();

        BOOST_REQUIRE_EQUAL(contenttypestream.str(),
                            dopamine::services::MIME_TYPE_APPLICATION_DICOM);

        // check Body
        mimetic::Body& body = (*mbit)->body();

        // remove the ended boundary
        std::string temp(body.c_str(), body.size());
        temp = temp.substr(0, temp.rfind("\n\n--"));

        // remove ended '\n'
        while (temp[temp.size()-1] == '\n')
        {
            temp = temp.substr(0, temp.rfind("\n"));
        }

        // Create buffer for DCMTK
        DcmInputBufferStream* inputbufferstream = new DcmInputBufferStream();
        inputbufferstream->setBuffer(temp.c_str(), temp.size());
        inputbufferstream->setEos();

        // Convert buffer into Dataset
        DcmFileFormat fileformat;
        fileformat.transferInit();
        OFCondition condition = fileformat.read(*inputbufferstream);
        fileformat.transferEnd();

        delete inputbufferstream;
        BOOST_REQUIRE(condition.good());

        // check sop instance
        OFString sopinstanceuid;
        fileformat.getDataset()->findAndGetOFStringArray(DCM_SOPInstanceUID,
                                                         sopinstanceuid);
        BOOST_CHECK_EQUAL(std::string(sopinstanceuid.c_str()),
                          SOP_INSTANCE_UID_01_01_01_01);
    }
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: wado_rs Accessors
 */
BOOST_FIXTURE_TEST_CASE(Accessors, ServicesTestClass)
{
    std::stringstream stream;
    stream << "/";
    stream << "studies" << "/" << STUDY_INSTANCE_UID_01_01;

    dopamine::services::Wado_rs wadors(stream.str());

    BOOST_CHECK_NE(wadors.get_boundary(), "");
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: wado_rs request (Study)
 */
BOOST_FIXTURE_TEST_CASE(RequestStudy, ServicesTestClass)
{
    std::stringstream stream;
    stream << "/";
    stream << "studies" << "/" << STUDY_INSTANCE_UID_01_01;

    // Create the response
    dopamine::services::Wado_rs wadors(stream.str());

    check_response(wadors.get_response(), wadors.get_boundary());
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: wado_rs request (Study/Series)
 */
BOOST_FIXTURE_TEST_CASE(RequestStudySeries, ServicesTestClass)
{
    std::stringstream stream;
    stream << "/";
    stream << "studies" << "/" << STUDY_INSTANCE_UID_01_01 << "/";
    stream << "series" << "/" << SERIES_INSTANCE_UID_01_01_01;

    // Create the response
    dopamine::services::Wado_rs wadors(stream.str());

    check_response(wadors.get_response(), wadors.get_boundary());
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: wado_rs request (Study/Series/Instance)
 */
BOOST_FIXTURE_TEST_CASE(RequestStudySeriesInstance, ServicesTestClass)
{
    std::stringstream stream;
    stream << "/";
    stream << "studies" << "/" << STUDY_INSTANCE_UID_01_01 << "/";
    stream << "series" << "/" << SERIES_INSTANCE_UID_01_01_01 << "/";
    stream << "instances" << "/" << SOP_INSTANCE_UID_01_01_01_01;

    // Create the response
    dopamine::services::Wado_rs wadors(stream.str());

    check_response(wadors.get_response(), wadors.get_boundary());
}

/*************************** TEST Error *********************************/
/**
 * Error test case: No parameter
 */
BOOST_AUTO_TEST_CASE(MissingStudyParameter)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });

    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Unknown first parameter
 */
BOOST_AUTO_TEST_CASE(UnknownFirstParameter)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/unknown/value", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Missing Study Instance UID
 */
BOOST_AUTO_TEST_CASE(MissingStudyInstance)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });

    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Unknown second parameter
 */
BOOST_AUTO_TEST_CASE(UnknownSecondParameter)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/unknown/value", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Missing Series Instance UID
 */
BOOST_AUTO_TEST_CASE(MissingSeriesInstance)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/series/", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });

    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/series", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Unknown third parameter
 */
BOOST_AUTO_TEST_CASE(UnknownThirdParameter)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/series/value/unknown/value", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Missing SOP Instance UID
 */
BOOST_AUTO_TEST_CASE(MissingSOPInstance)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/series/value/instances/", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });

    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/series/value/instances", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 400 && exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: dataset not find
 */
BOOST_FIXTURE_TEST_CASE(DatasetNotFind, ServicesTestClass)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/series/value/instances/value", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 404 && exc.statusmessage() == "Not Found"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: No database
 */
BOOST_AUTO_TEST_CASE(DatabaseNotConnected)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs("/studies/value/series/value/instances/value", ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 500 && exc.statusmessage() == "Internal Server Error"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: dataset cannot be return
 */
BOOST_FIXTURE_TEST_CASE(BadDatasetBufferValue, ServicesTestClass)
{
    std::stringstream stream;
    stream << "/";
    stream << "studies" << "/" << "2.16.756.5.5.100.3611280983.20092.123456789" << "/";
    stream << "series" << "/" << "2.16.756.5.5.100.3611280983.20092.123456789.0" << "/";
    stream << "instances" << "/" << "2.16.756.5.5.100.3611280983.20092.123456789.0.0";

    BOOST_CHECK_EXCEPTION(dopamine::services::Wado_rs(stream.str(), ""),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
        { return (exc.status() == 500 && exc.statusmessage() == "Internal Server Error"); });
}
