/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#define BOOST_TEST_MODULE ModuleXMLToBSON
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/test/unit_test.hpp>

#include "core/ExceptionPACS.h"
#include "ConverterBSON/XML/XMLToBSON.h"
#include "services/ServicesTools.h"

/****************************** TOOLS ***********************************/

template<typename TType>
void create_test_value(std::string const & dicom_tag,
                       std::string const & dicom_vr,
                       std::string const & dicom_keyword,
                       std::vector<TType> const & values)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, dicom_tag);
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, dicom_vr);
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       dicom_keyword);

    unsigned int count = 0;
    for (auto value : values)
    {
        ++count;
        std::stringstream number;
        number << count;
        boost::property_tree::ptree tagvalue;
        tagvalue.put(dopamine::converterBSON::Attribute_Number, number.str());
        tagvalue.put_value<TType>(value);

        dicomattribute.add_child(dopamine::converterBSON::Tag_Value, tagvalue);
    }

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    mongo::BSONObj const object = xmltobson.from_ptree(tree);

    // Check result
    BOOST_CHECK_EQUAL(object.hasField(dicom_tag), true);
    BOOST_CHECK_EQUAL(object.getField(dicom_tag).type(),
                      mongo::BSONType::Object);
    mongo::BSONObj const subobject = object.getField(dicom_tag).Obj();
    BOOST_CHECK_EQUAL(subobject.hasField("vr"), true);
    BOOST_CHECK_EQUAL(subobject.getField("vr").String(), dicom_vr);
    BOOST_CHECK_EQUAL(subobject.getField("Value").type(),
                      mongo::BSONType::Array);
    auto array = subobject.getField("Value").Array();
    BOOST_CHECK_EQUAL(array.size(), values.size());

    count = 0;
    for (auto value : array)
    {
        std::string const result_value =
                dopamine::services::bsonelement_to_string(value);
        std::stringstream initial_value;
        initial_value << values[count];
        BOOST_CHECK_EQUAL(result_value, initial_value.str());

        ++count;
    }
}

void create_test_inlinebinary(std::string const & dicom_tag,
                              std::string const & dicom_vr,
                              std::string const & dicom_keyword,
                              std::string const & value)
{
    boost::property_tree::ptree inlinebinary;
    inlinebinary.put_value<std::string>(value);

    boost::property_tree::ptree dicomattributeBinary;
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_Tag, dicom_tag);
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_VR, dicom_vr);
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_Keyword,
                             dicom_keyword);
    dicomattributeBinary.add_child(dopamine::converterBSON::Tag_InlineBinary,
                                   inlinebinary);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattributeBinary);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    mongo::BSONObj const object_bson = xmltobson.from_ptree(tree);

    BOOST_CHECK_EQUAL(object_bson.hasField(dicom_tag), true);
    BOOST_CHECK_EQUAL(object_bson.getField(dicom_tag).type(),
                      mongo::BSONType::Object);
    mongo::BSONObj const subobject = object_bson.getField(dicom_tag).Obj();
    BOOST_CHECK_EQUAL(subobject.hasField("vr"), true);
    BOOST_CHECK_EQUAL(subobject.getField("vr").String(), dicom_vr);
    BOOST_CHECK_EQUAL(subobject.hasField("InlineBinary"), true);
    BOOST_CHECK_EQUAL(subobject.getField("InlineBinary").type(),
                      mongo::BSONType::BinData);

    int size=0;
    char const * begin2 = subobject.getField("InlineBinary").binDataClean(size);
    std::string result(begin2, size);

    std::string const realvalue = "azertyuiopqsdfghjklmwxcvbn123456";
    BOOST_CHECK_EQUAL(result, realvalue);
}

struct component_name
{
    std::string _first_name;
    std::string _given_name;
    std::string _middle_name;
    std::string _prefix;
    std::string _suffix;
    std::string _tag;

    component_name(std::string const & firstname,
                   std::string const & givenname,
                   std::string const & middlename,
                   std::string const & prefix,
                   std::string const & suffix,
                   std::string const & tag):
        _first_name(firstname), _given_name(givenname),
        _middle_name(middlename), _prefix(prefix),
        _suffix(suffix), _tag(tag) {}
};

void create_test_personname(std::string const & dicom_tag,
                            std::string const & dicom_keyword,
                            std::vector<component_name> names)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, dicom_tag);
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       dicom_keyword);

    unsigned int count = 0;
    for (auto name : names)
    {
        ++count;
        std::stringstream number;
        number << count;

        boost::property_tree::ptree componentname;
        boost::property_tree::ptree firstname;
        firstname.put_value<std::string>(name._first_name);
        componentname.add_child(dopamine::converterBSON::Tag_FamilyName,
                                firstname);
        boost::property_tree::ptree givenname;
        givenname.put_value<std::string>(name._given_name);
        componentname.add_child(dopamine::converterBSON::Tag_GivenName,
                                givenname);
        boost::property_tree::ptree middlename;
        middlename.put_value<std::string>(name._middle_name);
        componentname.add_child(dopamine::converterBSON::Tag_MiddleName,
                                middlename);
        boost::property_tree::ptree prefix;
        prefix.put_value<std::string>(name._prefix);
        componentname.add_child(dopamine::converterBSON::Tag_NamePrefix,
                                prefix);
        boost::property_tree::ptree suffix;
        suffix.put_value<std::string>(name._suffix);
        componentname.add_child(dopamine::converterBSON::Tag_NameSuffix,
                                suffix);

        boost::property_tree::ptree personname;
        personname.put(dopamine::converterBSON::Attribute_Number, number.str());
        personname.add_child(name._tag, componentname);

        dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                                 personname);
    }

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    mongo::BSONObj const object = xmltobson.from_ptree(tree);

    // Check result
    BOOST_CHECK_EQUAL(object.hasField(dicom_tag), true);
    BOOST_CHECK_EQUAL(object.getField(dicom_tag).type(),
                      mongo::BSONType::Object);
    mongo::BSONObj const subobject = object.getField(dicom_tag).Obj();
    BOOST_CHECK_EQUAL(subobject.hasField("vr"), true);
    BOOST_CHECK_EQUAL(subobject.getField("vr").String(), "PN");
    BOOST_CHECK_EQUAL(subobject.getField("Value").type(),
                      mongo::BSONType::Array);
    auto array = subobject.getField("Value").Array();
    BOOST_CHECK_EQUAL(array.size(), names.size());

    count = 0;
    for (auto value : array)
    {
        BOOST_CHECK_EQUAL(value.type(),  mongo::BSONType::Object);

        std::string const search_type = names[count]._tag;

        mongo::BSONObj const objectname = value.Obj();
        BOOST_CHECK_EQUAL(objectname.hasField(search_type), true);
        BOOST_CHECK_EQUAL(objectname.getField(search_type).type(),
                          mongo::BSONType::String);

        std::string const result_value =
                objectname.getField(search_type).String();

        std::vector<std::string> name_components;
        boost::split(name_components, result_value,
                     boost::is_any_of("^"), boost::token_compress_off);
        BOOST_CHECK_EQUAL(name_components.size(), 5);
        BOOST_CHECK_EQUAL(name_components[0], names[count]._first_name);
        BOOST_CHECK_EQUAL(name_components[1], names[count]._given_name);
        BOOST_CHECK_EQUAL(name_components[2], names[count]._middle_name);
        BOOST_CHECK_EQUAL(name_components[3], names[count]._prefix);
        BOOST_CHECK_EQUAL(name_components[4], names[count]._suffix);

        ++count;
    }
}

void create_test_sequence(std::string const & dicom_tag,
                          std::string const & dicom_keyword,
                          std::vector<boost::property_tree::ptree> tagitems)
{
    boost::property_tree::ptree dicomattributeSQ;
    dicomattributeSQ.put(dopamine::converterBSON::Attribute_Tag, dicom_tag);
    dicomattributeSQ.put(dopamine::converterBSON::Attribute_VR, "SQ");
    dicomattributeSQ.put(dopamine::converterBSON::Attribute_Keyword,
                         dicom_keyword);

    unsigned int count = 0;
    for (auto tagitem : tagitems)
    {
        ++count;
        std::stringstream number;
        number << count;

        boost::property_tree::ptree item;
        item.put(dopamine::converterBSON::Attribute_Number, number.str());
        item.add_child(dopamine::converterBSON::Tag_DicomAttribute, tagitem);

        dicomattributeSQ.add_child(dopamine::converterBSON::Tag_Item, item);
    }

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattributeSQ);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    mongo::BSONObj const object = xmltobson.from_ptree(tree);

    // Check result
    BOOST_CHECK_EQUAL(object.hasField(dicom_tag), true);
    BOOST_CHECK_EQUAL(object.getField(dicom_tag).type(),
                      mongo::BSONType::Object);
    mongo::BSONObj const subobject = object.getField(dicom_tag).Obj();
    BOOST_CHECK_EQUAL(subobject.hasField("vr"), true);
    BOOST_CHECK_EQUAL(subobject.getField("vr").String(), "SQ");
    BOOST_CHECK_EQUAL(subobject.getField("Value").type(),
                      mongo::BSONType::Array);
    auto array = subobject.getField("Value").Array();
    BOOST_CHECK_EQUAL(array.size(), tagitems.size());

    count = 0;
    for (auto value : array)
    {
        BOOST_CHECK_EQUAL(value.type(),  mongo::BSONType::Object);

        mongo::BSONObj const objectitem = value.Obj();

        std::string const name =
                tagitems[count].get<std::string>(
                    dopamine::converterBSON::Attribute_Tag);
        std::string const vr =
                tagitems[count].get<std::string>(
                    dopamine::converterBSON::Attribute_VR);

        BOOST_CHECK_EQUAL(objectitem.hasField(name), true);
        BOOST_CHECK_EQUAL(objectitem.getField(name).type(),
                          mongo::BSONType::Object);
        mongo::BSONObj const subobjectitem = objectitem.getField(name).Obj();
        BOOST_CHECK_EQUAL(subobjectitem.hasField("vr"), true);
        BOOST_CHECK_EQUAL(subobjectitem.getField("vr").String(), vr);
        BOOST_CHECK_EQUAL(subobjectitem.getField("Value").type(),
                          mongo::BSONType::Array);

        ++count;
    }
}

/**************************** END TOOLS *********************************/


/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Constructor / Destructor
 */
BOOST_AUTO_TEST_CASE(Constructor)
{
    dopamine::converterBSON::XMLToBSON * xmltobson =
            new dopamine::converterBSON::XMLToBSON();

    BOOST_CHECK_EQUAL(xmltobson != NULL, true);

    delete xmltobson;
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR AE
 */
BOOST_AUTO_TEST_CASE(ConversionAE)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00080054", "AE", "RetrieveAETitle", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR AS
 */
BOOST_AUTO_TEST_CASE(ConversionAS)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00101010", "AS", "PatientAge", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR AT
 */
BOOST_AUTO_TEST_CASE(ConversionAT)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00209165", "AT", "DimensionIndexPointer", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR CS
 */
BOOST_AUTO_TEST_CASE(ConversionCS)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00080060", "CS", "Modality", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR DA
 */
BOOST_AUTO_TEST_CASE(ConversionDA)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00100030", "DA", "PatientBirthDate", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR DS
 */
BOOST_AUTO_TEST_CASE(ConversionDS)
{
    std::vector<Float64> values = { 11.11, 22.22 };
    create_test_value("00101030", "DS", "PatientWeight", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR DT
 */
BOOST_AUTO_TEST_CASE(ConversionDT)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00189074", "DT", "FrameAcquisitionDateTime", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR FD
 */
BOOST_AUTO_TEST_CASE(ConversionFD)
{
    std::vector<Float64> values = { 44.4, 55.5 };
    create_test_value("00460044", "FD", "PupilSize", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR FL
 */
BOOST_AUTO_TEST_CASE(ConversionFL)
{
    std::vector<Float32> values = { 77.7, 88.8 };
    create_test_value("00089459", "FL", "RecommendedDisplayFrameRateInFloat",
                      values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR IS
 */
BOOST_AUTO_TEST_CASE(ConversionIS)
{
    std::vector<Sint32> values = { 111, 222 };
    create_test_value("00082122", "IS", "StageNumber", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR LO
 */
BOOST_AUTO_TEST_CASE(ConversionLO)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00080070", "LO", "Manufacturer", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR LT
 */
BOOST_AUTO_TEST_CASE(ConversionLT)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("001021b0", "LT", "AdditionalPatientHistory", values);
}


/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR OB
 */
BOOST_AUTO_TEST_CASE(ConversionOB)
{
    std::string const value = "YXplcnR5dWlvcHFzZGZnaGprbG13eGN2Ym4xMjM0NTY";
    create_test_inlinebinary("00282000", "OB", "ICCProfile", value);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR OF
 */
BOOST_AUTO_TEST_CASE(ConversionOF)
{
    std::string const value = "YXplcnR5dWlvcHFzZGZnaGprbG13eGN2Ym4xMjM0NTY";
    create_test_inlinebinary("00640009", "OF", "VectorGridData", value);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR OW
 */
BOOST_AUTO_TEST_CASE(ConversionOW)
{
    std::string const value = "YXplcnR5dWlvcHFzZGZnaGprbG13eGN2Ym4xMjM0NTY";
    create_test_inlinebinary("00660023", "OW", "TrianglePointIndexList", value);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR PN
 */
BOOST_AUTO_TEST_CASE(ConversionPN)
{
    std::vector<component_name> values =
    { component_name("Doe", "John", "Wallas", "Rev.",
                     "Chief Executive Officer",
                     dopamine::converterBSON::Tag_Alphabetic),
      component_name("Smith", "Jane", "Scarlett", "Ms.", "Goddess",
                     dopamine::converterBSON::Tag_Alphabetic)
    };
    create_test_personname("00100010", "PatientName", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR SH
 */
BOOST_AUTO_TEST_CASE(ConversionSH)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00102160", "SH", "EthnicGroup", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR SL
 */
BOOST_AUTO_TEST_CASE(ConversionSL)
{
    std::vector<Sint32> values = { 1001, 2002 };
    create_test_value("00186020", "SL", "ReferencePixelX0", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR SQ
 */
BOOST_AUTO_TEST_CASE(ConversionSQ)
{
    boost::property_tree::ptree value;
    value.put(dopamine::converterBSON::Attribute_Number, "1");
    value.put_value<std::string>("valueLO1");

    boost::property_tree::ptree attributeLO;
    attributeLO.put(dopamine::converterBSON::Attribute_Tag, "00100020");
    attributeLO.put(dopamine::converterBSON::Attribute_VR, "LO");
    attributeLO.put(dopamine::converterBSON::Attribute_Keyword, "PatientID");
    attributeLO.add_child(dopamine::converterBSON::Tag_Value, value);

    std::vector<boost::property_tree::ptree> values = { attributeLO,
                                                        attributeLO };
    create_test_sequence("00101002", "OtherPatientIDsSequence", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR SS
 */
BOOST_AUTO_TEST_CASE(ConversionSS)
{
    std::vector<Sint16> values = { 555, 666 };
    create_test_value("00189219", "SS", "TagAngleSecondAxis", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR ST
 */
BOOST_AUTO_TEST_CASE(ConversionST)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00080081", "ST", "InstitutionAddress", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR TM
 */
BOOST_AUTO_TEST_CASE(ConversionTM)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00080013", "TM", "InstanceCreationTime", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR UI
 */
BOOST_AUTO_TEST_CASE(ConversionUI)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00080016", "UI", "SOPClassUID", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR UL
 */
BOOST_AUTO_TEST_CASE(ConversionUL)
{
    std::vector<Uint32> values = { 555, 666 };
    create_test_value("00081161", "UL", "SimpleFrameList", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR UN
 */
BOOST_AUTO_TEST_CASE(ConversionUN)
{
    // TODO
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR US
 */
BOOST_AUTO_TEST_CASE(ConversionUS)
{
    std::vector<Uint16> values = { 77, 88 };
    create_test_value("00081197", "US", "FailureReason", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Conversion VR UT
 */
BOOST_AUTO_TEST_CASE(ConversionUT)
{
    std::vector<std::string> values = { "value01", "value02" };
    create_test_value("00287fe0", "UT", "PixelDataProviderURL", values);
}

/*************************** TEST Nominal *******************************/
/**
 * Nominal test case: Append null value
 */
BOOST_AUTO_TEST_CASE(Null_value)
{
    // Value
    {
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00080060");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "CS");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "Modality");

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel,
                   nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    mongo::BSONObj const object_value = xmltobson.from_ptree(tree);

    BOOST_CHECK(object_value["00080060"].Obj().getField("Value").isNull());
    }

    // InlineBinary
    {
    boost::property_tree::ptree dicomattributeBinary;
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_Tag,
                             "00282000");
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_VR, "OB");
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_Keyword,
                             "ICCProfile");

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattributeBinary);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel,
                   nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    mongo::BSONObj const object_bson = xmltobson.from_ptree(tree);

    BOOST_CHECK(object_bson["00282000"].Obj()["InlineBinary"].isNull());
    }
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Missing main node NativeDicomModel
 */
BOOST_AUTO_TEST_CASE(No_NativeDicomModel_Node)
{
    boost::property_tree::ptree nativetree;

    boost::property_tree::ptree tree;
    tree.add_child("BadNode", nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too many main node NativeDicomModel
 */
BOOST_AUTO_TEST_CASE(TooMany_NativeDicomModel_Node)
{
    boost::property_tree::ptree nativetree;

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Not a DicomAttribute node into NativeDicomModel
 */
BOOST_AUTO_TEST_CASE(Not_DicomAttribute_SubNode)
{
    boost::property_tree::ptree dicomattribute;

    boost::property_tree::ptree nativetree;
    nativetree.add_child("BadNode", dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Bad subnode into PersonName
 */
BOOST_AUTO_TEST_CASE(Bad_PersonName_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00100010");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "PatientName");

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree firstname;
    firstname.put_value<std::string>("first");
    componentname.add_child(dopamine::converterBSON::Tag_FamilyName, firstname);
    boost::property_tree::ptree givenname;
    givenname.put_value<std::string>("given");
    componentname.add_child(dopamine::converterBSON::Tag_GivenName, givenname);
    boost::property_tree::ptree middlename;
    middlename.put_value<std::string>("middle");
    componentname.add_child(dopamine::converterBSON::Tag_MiddleName, middlename);
    boost::property_tree::ptree prefix;
    prefix.put_value<std::string>("prefix");
    componentname.add_child(dopamine::converterBSON::Tag_NamePrefix, prefix);
    boost::property_tree::ptree suffix;
    suffix.put_value<std::string>("suffix");
    componentname.add_child(dopamine::converterBSON::Tag_NameSuffix, suffix);

    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child("BadValue", componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Bad subnode into Sequence DicomAttribute
 */
BOOST_AUTO_TEST_CASE(Bad_Sequence_SubNode)
{
    boost::property_tree::ptree dicomattributeSQ;
    dicomattributeSQ.put(dopamine::converterBSON::Attribute_Tag, "00101002");
    dicomattributeSQ.put(dopamine::converterBSON::Attribute_VR, "SQ");
    dicomattributeSQ.put(dopamine::converterBSON::Attribute_Keyword,
                         "OtherPatientIDsSequence");

    boost::property_tree::ptree value;
    value.put(dopamine::converterBSON::Attribute_Number, "1");
    value.put_value<std::string>("valueLO1");

    boost::property_tree::ptree attributeLO;
    attributeLO.put(dopamine::converterBSON::Attribute_Tag, "00100020");
    attributeLO.put(dopamine::converterBSON::Attribute_VR, "LO");
    attributeLO.put(dopamine::converterBSON::Attribute_Keyword, "PatientID");
    attributeLO.add_child(dopamine::converterBSON::Tag_Value, value);

    boost::property_tree::ptree item;
    item.put(dopamine::converterBSON::Attribute_Number, "1");
    item.add_child(dopamine::converterBSON::Tag_DicomAttribute, attributeLO);

    dicomattributeSQ.add_child(dopamine::converterBSON::Tag_Value,
                               item); // BadValue

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattributeSQ);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Bad subnode into Alphabetic
 */
BOOST_AUTO_TEST_CASE(Bad_Alphabetic_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00100010");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "PatientName");

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree firstname;
    firstname.put_value<std::string>("first");
    componentname.add_child("BadValue", firstname);
    boost::property_tree::ptree givenname;
    givenname.put_value<std::string>("given");
    componentname.add_child(dopamine::converterBSON::Tag_GivenName, givenname);
    boost::property_tree::ptree middlename;
    middlename.put_value<std::string>("middle");
    componentname.add_child(dopamine::converterBSON::Tag_MiddleName, middlename);
    boost::property_tree::ptree prefix;
    prefix.put_value<std::string>("prefix");
    componentname.add_child(dopamine::converterBSON::Tag_NamePrefix, prefix);
    boost::property_tree::ptree suffix;
    suffix.put_value<std::string>("suffix");
    componentname.add_child(dopamine::converterBSON::Tag_NameSuffix, suffix);

    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child(dopamine::converterBSON::Tag_Alphabetic, componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too many FamilyName subnode into Alphabetic
 */
BOOST_AUTO_TEST_CASE(TooMany_FamilyName_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00100010");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "PatientName");

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree firstname;
    firstname.put_value<std::string>("one");
    componentname.add_child(dopamine::converterBSON::Tag_FamilyName, firstname);
    boost::property_tree::ptree givenname;
    givenname.put_value<std::string>("two");
    componentname.add_child(dopamine::converterBSON::Tag_FamilyName, givenname);

    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child(dopamine::converterBSON::Tag_Alphabetic, componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too many GivenName subnode into Alphabetic
 */
BOOST_AUTO_TEST_CASE(TooMany_GivenName_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00100010");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "PatientName");

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree firstname;
    firstname.put_value<std::string>("one");
    componentname.add_child(dopamine::converterBSON::Tag_GivenName, firstname);
    boost::property_tree::ptree givenname;
    givenname.put_value<std::string>("two");
    componentname.add_child(dopamine::converterBSON::Tag_GivenName, givenname);

    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child(dopamine::converterBSON::Tag_Alphabetic, componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too many MiddleName subnode into Alphabetic
 */
BOOST_AUTO_TEST_CASE(TooMany_MiddleName_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00100010");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "PatientName");

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree firstname;
    firstname.put_value<std::string>("one");
    componentname.add_child(dopamine::converterBSON::Tag_MiddleName, firstname);
    boost::property_tree::ptree givenname;
    givenname.put_value<std::string>("two");
    componentname.add_child(dopamine::converterBSON::Tag_MiddleName, givenname);

    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child(dopamine::converterBSON::Tag_Alphabetic, componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too many NamePrefix subnode into Alphabetic
 */
BOOST_AUTO_TEST_CASE(TooMany_NamePrefix_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00100010");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "PatientName");

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree firstname;
    firstname.put_value<std::string>("one");
    componentname.add_child(dopamine::converterBSON::Tag_NamePrefix, firstname);
    boost::property_tree::ptree givenname;
    givenname.put_value<std::string>("two");
    componentname.add_child(dopamine::converterBSON::Tag_NamePrefix, givenname);

    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child(dopamine::converterBSON::Tag_Alphabetic, componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too many NameSuffix subnode into Alphabetic
 */
BOOST_AUTO_TEST_CASE(TooMany_NameSuffix_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00100010");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "PN");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "PatientName");

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree firstname;
    firstname.put_value<std::string>("one");
    componentname.add_child(dopamine::converterBSON::Tag_NameSuffix, firstname);
    boost::property_tree::ptree givenname;
    givenname.put_value<std::string>("two");
    componentname.add_child(dopamine::converterBSON::Tag_NameSuffix, givenname);

    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child(dopamine::converterBSON::Tag_Alphabetic, componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Bad Attribute Tag
 */
BOOST_AUTO_TEST_CASE(Bad_DicomAttribute_Tag)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00xx0024");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "CS");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword, "Unknown");

    boost::property_tree::ptree tagvalue;
    tagvalue.put(dopamine::converterBSON::Attribute_Number, "1");
    tagvalue.put_value<std::string>("value");

    dicomattribute.add_child(dopamine::converterBSON::Tag_Value, tagvalue);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Not a valid subnode into DicomAttribute
 */
BOOST_AUTO_TEST_CASE(Not_Value_SubNode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00080060");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "CS");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword, "MR");

    boost::property_tree::ptree tagvalue;
    tagvalue.put(dopamine::converterBSON::Attribute_Number, "1");
    tagvalue.put_value<std::string>("value");

    dicomattribute.add_child("BadValue", tagvalue);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too Many different subnode into DicomAttribute
 */
BOOST_AUTO_TEST_CASE(TooMany_Different_Subnode)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00080060");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "CS");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword, "MR");

    boost::property_tree::ptree tagvalue;
    tagvalue.put(dopamine::converterBSON::Attribute_Number, "1");
    tagvalue.put_value<std::string>("value");

    dicomattribute.add_child(dopamine::converterBSON::Tag_Value, tagvalue);

    boost::property_tree::ptree componentname;
    boost::property_tree::ptree personname;
    personname.put(dopamine::converterBSON::Attribute_Number, "1");
    personname.add_child(dopamine::converterBSON::Tag_Alphabetic, componentname);

    dicomattribute.add_child(dopamine::converterBSON::Tag_PersonName,
                             personname);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Too Many InlineBinary subnode into DicomAttribute
 */
BOOST_AUTO_TEST_CASE(TooMany_InlineBinary_Subnode)
{
    boost::property_tree::ptree inlinebinary;
    inlinebinary.put_value<std::string>(
                "YXplcnR5dWlvcHFzZGZnaGprbG13eGN2Ym4xMjM0NTY");

    boost::property_tree::ptree dicomattributeBinary;
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_Tag,
                             "00282000");
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_VR, "OB");
    dicomattributeBinary.put(dopamine::converterBSON::Attribute_Keyword,
                             "ICCProfile");
    dicomattributeBinary.add_child(dopamine::converterBSON::Tag_InlineBinary,
                                   inlinebinary);
    dicomattributeBinary.add_child(dopamine::converterBSON::Tag_InlineBinary,
                                   inlinebinary);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattributeBinary);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}

/*************************** TEST Error *********************************/
/**
 * Error test case: Unkown VR
 */
BOOST_AUTO_TEST_CASE(Unknown_VR)
{
    boost::property_tree::ptree dicomattribute;
    dicomattribute.put(dopamine::converterBSON::Attribute_Tag, "00140024");
    dicomattribute.put(dopamine::converterBSON::Attribute_VR, "ST");
    dicomattribute.put(dopamine::converterBSON::Attribute_Keyword,
                       "ComponentReferenceSystem");

    boost::property_tree::ptree tagvalue;
    tagvalue.put(dopamine::converterBSON::Attribute_Number, "1");
    tagvalue.put_value<std::string>("value");

    dicomattribute.add_child(dopamine::converterBSON::Tag_Value, tagvalue);

    boost::property_tree::ptree nativetree;
    nativetree.add_child(dopamine::converterBSON::Tag_DicomAttribute,
                         dicomattribute);

    boost::property_tree::ptree tree;
    tree.add_child(dopamine::converterBSON::Tag_NativeDicomModel, nativetree);

    // Conversion
    dopamine::converterBSON::XMLToBSON xmltobson;
    BOOST_REQUIRE_THROW(xmltobson.from_ptree(tree), dopamine::ExceptionPACS);
}