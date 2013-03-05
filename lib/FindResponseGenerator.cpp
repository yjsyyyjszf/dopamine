#include "FindResponseGenerator.h"

#include <fstream>
#include <set>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmnet/dicom.h>
#include <dcmtk/dcmnet/dimse.h>
#include <dcmtk/ofstd/ofcond.h>

#include <mongo/bson/bson.h>
#include <mongo/client/dbclient.h>

#include "BSONToDataSet.h"
#include "DataSetToBSON.h"
#include "TagMatch.h"

std::string replace(std::string const & value, std::string const & old, 
                    std::string const & new_)
{
    std::string result(value);
    size_t begin=0;
    while(std::string::npos != (begin=result.find(old, begin)))
    {
        result = result.replace(begin, old.size(), new_);
        begin = (begin+new_.size()<result.size())?begin+new_.size()
                                                 :std::string::npos;
    }
    
    return result;
}

// Define Unknown specialization first, since other specializations use it.
template<>
void
FindResponseGenerator
::_dicom_query_to_mongo_query<FindResponseGenerator::Match::Unknown>(
    std::string const & field_name, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    // Default action: convert to string
    builder << field_name << value.String();
}

template<>
void
FindResponseGenerator
::_dicom_query_to_mongo_query<FindResponseGenerator::Match::SingleValue>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    builder << field << value.String();
}

template<>
void
FindResponseGenerator
::_dicom_query_to_mongo_query<FindResponseGenerator::Match::ListOfUID>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    mongo::BSONArrayBuilder or_builder;
    std::vector<mongo::BSONElement> or_terms = value.Array();
    for(std::vector<mongo::BSONElement>::const_iterator or_it=or_terms.begin();
        or_it!=or_terms.end(); ++or_it)
    {
        or_builder << BSON(field << (*or_it));
    }
    builder << "$or" << or_builder.arr();
}

template<>
void
FindResponseGenerator
::_dicom_query_to_mongo_query<FindResponseGenerator::Match::Universal>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    // Universal is not part of the MongoDB query : do nothing
}

template<>
void
FindResponseGenerator
::_dicom_query_to_mongo_query<FindResponseGenerator::Match::WildCard>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    // Convert DICOM regex to PCRE: replace "*" by ".*", "?" by ".",
    // and escape other special PCRE characters (these are :
    // \^$.[]()+{}
    //
    std::string regex = value.String();
    // Escape "." first since we're using it to replace "*"
    regex = replace(regex, ".", "\\.");
    regex = replace(regex, "*", ".*");
    regex = replace(regex, "?", ".");
    // Escape other PCRE metacharacters
    regex = replace(regex, "\\", "\\\\");
    regex = replace(regex, "^", "\\^");
    regex = replace(regex, "$", "\\$");
    regex = replace(regex, "[", "\\[");
    regex = replace(regex, "]", "\\]");
    regex = replace(regex, "(", "\\(");
    regex = replace(regex, ")", "\\)");
    regex = replace(regex, "+", "\\+");
    regex = replace(regex, "{", "\\{");
    regex = replace(regex, "}", "\\}");
    // Add the start and end anchors
    regex = "^"+regex+"$";

    // Use case-insensitive match for PN
    builder.appendRegex(field, regex, (vr=="PN")?"i":"");
}

template<>
void
FindResponseGenerator
::_dicom_query_to_mongo_query<FindResponseGenerator::Match::MultipleValues>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    mongo::BSONArrayBuilder or_builder;
    std::vector<mongo::BSONElement> or_terms = value.Array();
    for(std::vector<mongo::BSONElement>::const_iterator or_it=or_terms.begin();
        or_it!=or_terms.end(); ++or_it)
    {
        Match::Type const match_type = this->_get_match_type(vr, *or_it);

        DicomQueryToMongoQuery function = this->_get_query_conversion(match_type);
        mongo::BSONObjBuilder term_builder;
        (this->*function)(field, vr, *or_it, term_builder);

        or_builder << term_builder.obj();
    }
    builder << "$or" << or_builder.arr();
}

// C.2.2.2.5 Range Matching
//   TODO
// C.2.2.2.6 Sequence Matching
//   TODO

FindResponseGenerator
::FindResponseGenerator(DcmDataset /*const*/ & query, // DcmDataset is not const-correct
                        mongo::DBClientConnection & connection,
                        std::string const & db_name)
{
    // Convert the dataset to BSON, excluding Query/Retrieve Level.
    DataSetToBSON dataset_to_bson;

    dataset_to_bson.get_filters().push_back(
        std::make_pair(TagMatch::New(DCM_QueryRetrieveLevel),
                       DataSetToBSON::FilterAction::EXCLUDE));
    dataset_to_bson.get_filters().push_back(
        std::make_pair(TagMatch::New(DCM_SpecificCharacterSet),
                       DataSetToBSON::FilterAction::EXCLUDE));
    dataset_to_bson.set_default_filter(DataSetToBSON::FilterAction::INCLUDE);

    mongo::BSONObjBuilder query_builder;
    dataset_to_bson(&query, query_builder);
    mongo::BSONObj const query_dataset = query_builder.obj();

    // Build the MongoDB query and query fields from the query dataset.
    mongo::BSONObjBuilder db_query;
    mongo::BSONObjBuilder fields_builder;
    for(mongo::BSONObj::iterator it=query_dataset.begin(); it.more();)
    {
        mongo::BSONElement const element = it.next();
        std::vector<mongo::BSONElement> const array = element.Array();

        // Always include the field in the results
        fields_builder << element.fieldName() << 1;

        std::string const vr = array[0].String();
        mongo::BSONElement const & value = array[1];
        Match::Type const match_type = this->_get_match_type(vr, value);

        DicomQueryToMongoQuery function = this->_get_query_conversion(match_type);
        (this->*function)(element.fieldName(), vr, value, db_query);
    }

    // Always include Specific Character Set in results.
    if(!fields_builder.hasField("00080005"))
    {
        fields_builder << "00080005" << 1;
    }

    // Always include the keys for the query level.
    OFString ofstring;
    query.findAndGetOFString(DCM_QueryRetrieveLevel, ofstring);
    this->_query_retrieve_level = std::string(ofstring.c_str());
    if(this->_query_retrieve_level=="PATIENT " && !fields_builder.hasField("00100020"))
    {
        fields_builder << "00100020" << 1;
    }

    // Perform the DB query.
    mongo::BSONObj const fields = fields_builder.obj();

    mongo::BSONObj group_command = BSON("group" << BSON(
        "ns" << "datasets" <<
        "key" << fields <<
        "$reduce" << "function(x,y) {}" <<
        "initial" << mongo::BSONObj() <<
        "cond" << db_query.obj()
    ));
    connection.runCommand(db_name, group_command, this->_info, 0);
    this->_results = this->_info["retval"].Array();
    this->_index = 0;

    this->_status = STATUS_Pending;
}

DIC_US
FindResponseGenerator
::status() const
{
    return this->_status;
}

OFCondition
FindResponseGenerator
::next(DcmDataset ** responseIdentifiers)
{
    OFCondition cond;

    if(this->_index == this->_results.size())
    {
        // We're done.
        this->_status = STATUS_Success;
    }
    else
    {
        BSONToDataSet bson_to_dataset;
        mongo::BSONObj item = this->_results[this->_index].Obj();
        DcmDataset dataset = bson_to_dataset(item);
        ++this->_index;

        dataset.putAndInsertOFStringArray(DCM_QueryRetrieveLevel,
                                          this->_query_retrieve_level.c_str());

        (*responseIdentifiers) = new DcmDataset(dataset);

        this->_status = STATUS_Pending;
    }

    return cond;
}

void
FindResponseGenerator
::cancel()
{
    std::cout << "TODO : FindResponseGenerator::cancel()" << std::endl;
}

FindResponseGenerator::Match::Type 
FindResponseGenerator
::_get_match_type(std::string const & vr, 
                  mongo::BSONElement const & element) const
{
    Match::Type type = Match::Unknown;
    
    if(element.isNull())
    {
        // C.2.2.2.3 Universal Matching
        // Value is zero-length
        type = Match::Universal;
    }
    else
    {
        bool const is_date_or_time = (vr == "DA" || vr == "DT" || vr == "TM");
        bool const has_wildcard_matching = (!is_date_or_time &&
            vr != "SL" && vr != "SS" && vr != "UL" && vr != "UL" &&
            vr != "FD" && vr != "FL" &&
            vr != "OB" && vr != "OF" && vr != "OW" && vr != "UN" &&
            vr != "DS" && vr != "US" &&
            vr != "UI");
        if(element.type() == mongo::String)
        {
            std::string value(element);
            // Not a date or time, no wildcard AND date or time, no range
            if(!(!is_date_or_time && value.find_first_of("?*") != std::string::npos) && 
               !(is_date_or_time && value.find('-') != std::string::npos))
            {
                // C.2.2.2.1 Single Value Matching
                // Non-zero length AND 
                //   not a date or time or datetime, contains no wildcard OR
                //   a date or time or datetime, contains a single date or time 
                //   or datetime with not "-"
                type = Match::SingleValue;
            }
            else if(has_wildcard_matching && value.find_first_of("?*") != std::string::npos)
            {
                // C.2.2.2.4 Wild Card Matching
                // Not a date, time, datetime, SL, SL, UL, US, FL, FD, OB, 
                // OW, UN, AT, DS, IS, AS, UI and Value contains "*" or "?"
                type = Match::WildCard;
            }
            else if(is_date_or_time && value.find('-') != std::string::npos)
            {
                // C.2.2.2.5 Range Matching
                // Date, time or datetime, contains "-"
                type = Match::Range;
            }
        }
        else if(element.type() == mongo::Array && vr == "UI")
        {
            // C.2.2.2.2 List of UID Matching
            type = Match::ListOfUID;
        }
        else if(element.type() == mongo::Array && vr != "SQ")
        {
            type = Match::MultipleValues;
        }
    }
    
    // TODO : sequence matching
    // TODO : multiple values matching
    
    return type;
}

FindResponseGenerator::DicomQueryToMongoQuery
FindResponseGenerator
::_get_query_conversion(Match::Type const & match_type) const
{
    DicomQueryToMongoQuery function = NULL;
    if(match_type == Match::SingleValue)
    {
        function = &Self::_dicom_query_to_mongo_query<Match::SingleValue>;
    }
    else if(match_type == Match::ListOfUID)
    {
        function = &Self::_dicom_query_to_mongo_query<Match::ListOfUID>;
    }
    else if(match_type == Match::Universal)
    {
        function = &Self::_dicom_query_to_mongo_query<Match::Universal>;
    }
    else if(match_type == Match::WildCard)
    {
        function = &Self::_dicom_query_to_mongo_query<Match::WildCard>;
    }
    else if(match_type == Match::MultipleValues)
    {
        function = &Self::_dicom_query_to_mongo_query<Match::MultipleValues>;
    }
    else
    {
        function = &Self::_dicom_query_to_mongo_query<Match::Unknown>;
    }

    return function;
}
