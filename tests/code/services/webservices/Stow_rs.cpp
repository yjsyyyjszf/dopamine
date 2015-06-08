/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#define BOOST_TEST_MODULE ModuleQido_rs

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <dcmtk/config/osconfig.h> /* make sure OS specific configuration is included first */
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcistrmb.h>

#include "services/ServicesTools.h"
#include "services/webservices/Stow_rs.h"
#include "services/webservices/WebServiceException.h"
#include "../ServicesTestClass.h"

class TestDataRequest : public ServicesTestClass
{
public:
    std::string path_info;
    std::string content_type;
    std::string boundary;

    TestDataRequest() : ServicesTestClass()
    {
        path_info = "/studies";
        boundary = "4EMVgTUJNpDOGeP";

        std::stringstream content_typestream;
        content_typestream << dopamine::services::CONTENT_TYPE
                           << dopamine::services::MIME_TYPE_MULTIPART_RELATED << "; type="
                           << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "; "
                           << dopamine::services::ATTRIBUT_BOUNDARY << boundary;
        content_type = content_typestream.str();
    }

    virtual ~TestDataRequest()
    {
        // Nothing to do
    }
};

class TestDataRequestNotAllow : public TestDataRequest
{
public:
    TestDataRequestNotAllow():
        TestDataRequest()
    {
        mongo::BSONObjBuilder builder;
        builder.appendRegex("00080018", "Unknown");
        this->set_authorization("Store", "root", builder.obj());

        mongo::BSONObjBuilder builder2;
        builder2 << "00080060" << "NotMR";
        this->add_constraint("Store", "not_me", builder2.obj());
    }

    virtual ~TestDataRequestNotAllow()
    {
        // Nothing to do
    }
};

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: qido_rs Accessors
 */
BOOST_FIXTURE_TEST_CASE(Accessors, TestDataRequest)
{
    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "--";

    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());

    BOOST_REQUIRE(stowrs.get_boundary() != "");
    BOOST_CHECK_EQUAL(stowrs.get_content_type(),
                      dopamine::services::MIME_TYPE_APPLICATION_DICOM);
    BOOST_CHECK_EQUAL(stowrs.get_status(), 200);
    BOOST_CHECK_EQUAL(stowrs.get_code(), "OK");
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: stow_rs insert 1 dataset
 */
BOOST_FIXTURE_TEST_CASE(InsertOneDICOM, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_03_01_01_01)), 0);

    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_05"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output;
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());

    BOOST_REQUIRE(stowrs.get_response() != "");

    boost::property_tree::ptree ptree;
    std::stringstream xmlstream;
    xmlstream << stowrs.get_response();
    boost::property_tree::read_xml(xmlstream, ptree);
    BOOST_CHECK_EQUAL(ptree.find("NativeDicomModel") != ptree.not_found(), true);

    // check mandatory tag
    BOOST_CHECK(xmlstream.str().find("tag=\"00081190\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081199\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081150\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081155\"") != std::string::npos);

    // check tag error is missing
    BOOST_CHECK(xmlstream.str().find("tag=\"00081198\"") == std::string::npos);

    // check values
    BOOST_CHECK(xmlstream.str().find("1.2.840.10008.5.1.4.1.1.4") !=
                std::string::npos);
    BOOST_CHECK(xmlstream.str().find(SOP_INSTANCE_UID_03_01_01_01) !=
                std::string::npos);

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_03_01_01_01)), 1);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: stow_rs insert 3 dataset
 */
BOOST_FIXTURE_TEST_CASE(InsertThreeDICOM, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_01)), 0);
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_02)), 0);
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_03)), 0);

    std::vector<std::string> files = { std::string(getenv("DOPAMINE_TEST_DICOMFILE_07")),
                                       std::string(getenv("DOPAMINE_TEST_DICOMFILE_08")),
                                       std::string(getenv("DOPAMINE_TEST_DICOMFILE_09")) };

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    for (std::string datasetfile : files)
    {
        dataset << "--" << boundary << "\n";
        dataset << dopamine::services::CONTENT_TYPE
                << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
        dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
                << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
        dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
                << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

        // Open file
        std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
        BOOST_REQUIRE(file.is_open());

        // get length of file:
        int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));

        std::string output(length, '\0');

        // read data as a block:
        file.read (&output[0], output.size());

        // Close file
        file.close();

        dataset << output;
        dataset << "\n" << "\n";
    }
    dataset << "--" << boundary << "--";

    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());

    BOOST_REQUIRE(stowrs.get_response() != "");

    boost::property_tree::ptree ptree;
    std::stringstream xmlstream;
    xmlstream << stowrs.get_response();
    boost::property_tree::read_xml(xmlstream, ptree);
    BOOST_CHECK_EQUAL(ptree.find("NativeDicomModel") != ptree.not_found(), true);

    // check mandatory tag
    BOOST_CHECK(xmlstream.str().find("tag=\"00081190\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081199\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081150\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081155\"") != std::string::npos);

    // check tag error is missing
    BOOST_CHECK(xmlstream.str().find("tag=\"00081198\"") == std::string::npos);

    // check values
    BOOST_CHECK(xmlstream.str().find("1.2.840.10008.5.1.4.1.1.4") !=
                std::string::npos);
    BOOST_CHECK(xmlstream.str().find(SOP_INSTANCE_UID_04_01_01_01) !=
                std::string::npos);
    BOOST_CHECK(xmlstream.str().find(SOP_INSTANCE_UID_04_01_01_02) !=
                std::string::npos);
    BOOST_CHECK(xmlstream.str().find(SOP_INSTANCE_UID_04_01_01_03) !=
                std::string::npos);

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_01)), 1);
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_02)), 1);
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_03)), 1);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: stow_rs insert DICOM already register
 */
BOOST_FIXTURE_TEST_CASE(DicomAlreadyRegister, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_01_01_01_01)), 1);

    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_01"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output;
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    // Then try to register another time
    dopamine::services::Stow_rs stowrsagain(path_info, "", dataset.str());
    BOOST_REQUIRE(stowrsagain.get_response() != "");

    boost::property_tree::ptree ptree2;
    std::stringstream xmlstream2;
    xmlstream2 << stowrsagain.get_response();
    boost::property_tree::read_xml(xmlstream2, ptree2);
    BOOST_CHECK_EQUAL(ptree2.find("NativeDicomModel") != ptree2.not_found(), true);

    // check mandatory tag
    BOOST_CHECK(xmlstream2.str().find("tag=\"00081199\"") != std::string::npos);

    // check tag error is missing
    BOOST_CHECK(xmlstream2.str().find("tag=\"00081198\"") == std::string::npos);

    // check values
    BOOST_CHECK(xmlstream2.str().find("1.2.840.10008.5.1.4.1.1.4") !=
                std::string::npos);
    BOOST_CHECK(xmlstream2.str().find(SOP_INSTANCE_UID_01_01_01_01) !=
                std::string::npos);

    // Check into database (always present)
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_01_01_01_01)), 1);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: stow_rs check return status code
 */
BOOST_FIXTURE_TEST_CASE(ReturnStatusCode, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_01)), 0);
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_02)), 0);
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_03)), 0);

    {
    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_08"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output;
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    // First, insert into database
    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());
    BOOST_REQUIRE(stowrs.get_response() != "");

    BOOST_CHECK_EQUAL(stowrs.get_status(), 200);
    BOOST_CHECK_EQUAL(stowrs.get_code(), "OK");

    // Check into database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_01)), 0);
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_02)), 1);
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_04_01_01_03)), 0);
    }

    {
    std::vector<std::string> files = { std::string(getenv("DOPAMINE_TEST_DICOMFILE_07")),
                                       std::string(getenv("DOPAMINE_TEST_DICOMFILE_08")),
                                       std::string(getenv("DOPAMINE_TEST_DICOMFILE_09")) };

    {
    std::stringstream dataset;
    dataset << content_type << "\n\n";
    int count = 0;
    for (std::string datasetfile : files)
    {
        dataset << "--" << boundary << "\n";
        dataset << dopamine::services::CONTENT_TYPE
                << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
        dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
                << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
        dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
                << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

        // Open file
        std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
        BOOST_REQUIRE(file.is_open());

        // get length of file:
        int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));

        std::string output(length, '\0');

        // read data as a block:
        file.read (&output[0], output.size());

        // Close file
        file.close();

        dataset << output;
        if (count == 1)
        {
            dataset << "error";
        }
        dataset << "\n" << "\n";
        ++count;
    }
    dataset << "--" << boundary << "--";

    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());
    BOOST_REQUIRE(stowrs.get_response() != "");

    BOOST_CHECK_EQUAL(stowrs.get_status(), 202);
    BOOST_CHECK_EQUAL(stowrs.get_code(), "Accepted");
    }

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_01)), 1);
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_02)), 1);
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_03)), 1);

    // All in error
    {
    std::stringstream dataset;
    dataset << content_type << "\n\n";
    for (std::string datasetfile : files)
    {
        dataset << "--" << boundary << "\n";
        dataset << dopamine::services::CONTENT_TYPE
                << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
        dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
                << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
        dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
                << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

        // Open file
        std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
        BOOST_REQUIRE(file.is_open());

        // get length of file:
        int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));

        std::string output(length, '\0');

        // read data as a block:
        file.read (&output[0], output.size());

        // Close file
        file.close();

        dataset << output;
        dataset << "error";

        dataset << "\n" << "\n";
    }
    dataset << "--" << boundary << "--";

    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());
    BOOST_REQUIRE(stowrs.get_response() != "");

    BOOST_CHECK_EQUAL(stowrs.get_status(), 409);
    BOOST_CHECK_EQUAL(stowrs.get_code(), "Conflict");
    }

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_01)), 1);
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_02)), 1);
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_04_01_01_03)), 1);
    }
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: stow_rs insert 1 dataset for 1 identified Study
 */
BOOST_FIXTURE_TEST_CASE(InsertDatasetWithStudyInstanceUID, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_03_01_01_01)), 0);

    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_05"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output;
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    std::stringstream path_info_study;
    path_info_study << path_info << "/" << STUDY_INSTANCE_UID_03_01;
    dopamine::services::Stow_rs stowrs(path_info_study.str(), "", dataset.str());

    BOOST_REQUIRE(stowrs.get_response() != "");

    boost::property_tree::ptree ptree;
    std::stringstream xmlstream;
    xmlstream << stowrs.get_response();
    boost::property_tree::read_xml(xmlstream, ptree);
    BOOST_CHECK_EQUAL(ptree.find("NativeDicomModel") != ptree.not_found(), true);

    // check mandatory tag
    BOOST_CHECK(xmlstream.str().find("tag=\"00081190\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081199\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081150\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081155\"") != std::string::npos);

    // check tag error is missing
    BOOST_CHECK(xmlstream.str().find("tag=\"00081198\"") == std::string::npos);

    // check values
    BOOST_CHECK(xmlstream.str().find("1.2.840.10008.5.1.4.1.1.4") !=
                std::string::npos);
    BOOST_CHECK(xmlstream.str().find(SOP_INSTANCE_UID_03_01_01_01) !=
                std::string::npos);

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_03_01_01_01)), 1);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: stow_rs insert 1 dataset for 1 wrong Study
 */
BOOST_FIXTURE_TEST_CASE(InsertDatasetWithWrongStudyInstanceUID, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_03_01_01_01)), 0);

    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_05"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output;
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    std::stringstream path_info_study;
    path_info_study << path_info << "/" << "bad_value";
    dopamine::services::Stow_rs stowrs(path_info_study.str(), "", dataset.str());

    BOOST_REQUIRE(stowrs.get_response() != "");

    boost::property_tree::ptree ptree;
    std::stringstream xmlstream;
    xmlstream << stowrs.get_response();
    boost::property_tree::read_xml(xmlstream, ptree);
    BOOST_CHECK_EQUAL(ptree.find("NativeDicomModel") != ptree.not_found(), true);

    // check mandatory tag
    BOOST_CHECK(xmlstream.str().find("tag=\"00081190\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081197\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081198\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081150\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081155\"") != std::string::npos);

    // check tag ReferencedSOPSequence is missing
    BOOST_CHECK(xmlstream.str().find("tag=\"00081199\"") == std::string::npos);

    // check values
    BOOST_CHECK(xmlstream.str().find("<Value number=\"1\">42752</Value>") !=
                std::string::npos); // error code A700
    BOOST_CHECK(xmlstream.str().find("1.2.840.10008.5.1.4.1.1.4") !=
                std::string::npos);
    BOOST_CHECK(xmlstream.str().find(SOP_INSTANCE_UID_03_01_01_01) !=
                std::string::npos);

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_03_01_01_01)), 0);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Bad content-type for the part
 */
BOOST_FIXTURE_TEST_CASE(BadPartContentType, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_03_01_01_01)), 0);

    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_05"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOMXML << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output;
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());
    BOOST_REQUIRE(stowrs.get_response() != "");

    boost::property_tree::ptree ptree;
    std::stringstream xmlstream;
    xmlstream << stowrs.get_response();
    boost::property_tree::read_xml(xmlstream, ptree);
    BOOST_CHECK_EQUAL(ptree.find("NativeDicomModel") != ptree.not_found(), true);

    // check mandatory tag
    BOOST_CHECK(xmlstream.str().find("tag=\"00081190\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081197\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081198\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081150\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081155\"") != std::string::npos);

    // check tag ReferencedSOPSequence is missing
    BOOST_CHECK(xmlstream.str().find("tag=\"00081199\"") == std::string::npos);

    // check values
    BOOST_CHECK(xmlstream.str().find("<Value number=\"1\">272</Value>") !=
                std::string::npos); // error code 0110

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_03_01_01_01)), 0);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Cannot read dataset
 */
BOOST_FIXTURE_TEST_CASE(UnableToReadDataset, TestDataRequest)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_03_01_01_01)), 0);

    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_05"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output << "BADEND";
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    dopamine::services::Stow_rs stowrs(path_info, "", dataset.str());

    BOOST_REQUIRE(stowrs.get_response() != "");

    boost::property_tree::ptree ptree;
    std::stringstream xmlstream;
    xmlstream << stowrs.get_response();
    boost::property_tree::read_xml(xmlstream, ptree);
    BOOST_CHECK_EQUAL(ptree.find("NativeDicomModel") != ptree.not_found(), true);

    // check mandatory tag
    BOOST_CHECK(xmlstream.str().find("tag=\"00081190\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081197\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081198\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081150\"") != std::string::npos);
    BOOST_CHECK(xmlstream.str().find("tag=\"00081155\"") != std::string::npos);

    // check tag ReferencedSOPSequence is missing
    BOOST_CHECK(xmlstream.str().find("tag=\"00081199\"") == std::string::npos);

    // check values
    BOOST_CHECK(xmlstream.str().find("<Value number=\"1\">42752</Value>") !=
                std::string::npos); // error code A700

    // Check into database
    BOOST_CHECK_EQUAL(connection.count(db_name + ".datasets",
                                       BSON("00080018.Value" <<
                                            SOP_INSTANCE_UID_03_01_01_01)), 0);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Content-type not supported
 */
BOOST_FIXTURE_TEST_CASE(TypeNotSupported, TestDataRequest)
{
    {
    std::stringstream dataset;
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_MULTIPART_RELATED << "; type="
            << dopamine::services::MIME_TYPE_APPLICATION_DICOMXML << "; "
            << dopamine::services::ATTRIBUT_BOUNDARY << boundary;
    dataset << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOMXML << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    dataset << "SOMETHING";
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs(path_info, "", dataset.str()),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 415 &&
                                      exc.statusmessage() == "Unsupported Media Type"); });
    }

    {
    std::stringstream dataset;
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_MULTIPART_RELATED << "; type="
            << dopamine::services::MIME_TYPE_APPLICATION_JSON << "; "
            << dopamine::services::ATTRIBUT_BOUNDARY << boundary;
    dataset << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_JSON << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    dataset << "SOMETHING";
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs(path_info, "", dataset.str()),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 415 &&
                                      exc.statusmessage() == "Unsupported Media Type"); });
    }
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Not allow
 */
BOOST_FIXTURE_TEST_CASE(NotAllowToStore, TestDataRequestNotAllow)
{
    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_03_01_01_01)), 0);

    std::string datasetfile(getenv("DOPAMINE_TEST_DICOMFILE_05"));

    std::stringstream dataset;
    dataset << content_type << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    // Open file
    std::ifstream file(datasetfile, std::ifstream::binary | std::ifstream::in);
    BOOST_REQUIRE(file.is_open());
    // get length of file:
    int length = boost::filesystem::file_size(boost::filesystem::path(datasetfile));
    std::string output(length, '\0');
    // read data as a block:
    file.read (&output[0], output.size());
    // Close file
    file.close();

    dataset << output;
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs(path_info, "", dataset.str(), "ME"),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 401 &&
                                      exc.statusmessage() == "Unauthorized"); });

    // Check SOP Instance UID not present in database
    BOOST_REQUIRE_EQUAL(connection.count(db_name + ".datasets",
                                         BSON("00080018.Value" <<
                                              SOP_INSTANCE_UID_03_01_01_01)), 0);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Bad parameter (first parameter should be studies)
 */
BOOST_FIXTURE_TEST_CASE(BadParameter, TestDataRequest)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs("/badValue", "", content_type),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 400 &&
                                      exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Missing parameter
 */
BOOST_FIXTURE_TEST_CASE(MissingParameter, TestDataRequest)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs("", "", content_type),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 400 &&
                                      exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too Many parameter
 */
BOOST_FIXTURE_TEST_CASE(TooManyParameter, TestDataRequest)
{
    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs("/studies/1.2.3/tooMany", "", content_type),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 400 &&
                                      exc.statusmessage() == "Bad Request"); });
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Bad Content-Type
 */
BOOST_FIXTURE_TEST_CASE(BadContentType, TestDataRequest)
{
    {
    std::stringstream dataset;
    dataset << dopamine::services::CONTENT_TYPE << "; "
            << dopamine::services::ATTRIBUT_BOUNDARY << boundary;
    dataset << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOMXML << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    dataset << "SOMETHING";
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs(path_info, "", dataset.str()),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 400 &&
                                      exc.statusmessage() == "Bad Request"); });
    }

    {
    std::stringstream dataset;
    dataset << dopamine::services::CONTENT_TYPE
            << "BADVALUE" << "; type="
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "; "
            << dopamine::services::ATTRIBUT_BOUNDARY << boundary;
    dataset << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_APPLICATION_DICOM << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    dataset << "SOMETHING";
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs(path_info, "", dataset.str()),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 400 &&
                                      exc.statusmessage() == "Bad Request"); });
    }
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Media type not supported
 */
BOOST_FIXTURE_TEST_CASE(UnknownMediaType, TestDataRequest)
{
    std::stringstream dataset;
    dataset << dopamine::services::CONTENT_TYPE
            << dopamine::services::MIME_TYPE_MULTIPART_RELATED << "; type="
            << "application/text" << "; "
            << dopamine::services::ATTRIBUT_BOUNDARY << boundary;
    dataset << "\n\n";
    dataset << "--" << boundary << "\n";
    dataset << dopamine::services::CONTENT_TYPE
            << "application/text" << "\n";
    dataset << dopamine::services::CONTENT_DISPOSITION_ATTACHMENT << " "
            << dopamine::services::ATTRIBUT_FILENAME << "myfile" << "\n";
    dataset << dopamine::services::CONTENT_TRANSFER_ENCODING
            << dopamine::services::TRANSFER_ENCODING_BINARY << "\n" << "\n";

    dataset << "SOMETHING";
    dataset << "\n" << "\n";
    dataset << "--" << boundary << "--";

    BOOST_CHECK_EXCEPTION(dopamine::services::Stow_rs(path_info, "", dataset.str()),
                          dopamine::services::WebServiceException,
                          [] (dopamine::services::WebServiceException const exc)
                            { return (exc.status() == 400 &&
                                      exc.statusmessage() == "Bad Request"); });
}
