/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#include "ConverterBSON/BSONToDataSet.h"
#include "ConverterBSON/DataSetToBSON.h"
#include "ConverterBSON/TagMatch.h"
#include "core/LoggerPACS.h"
#include "Generator.h"
#include "services/ServicesTools.h"

namespace dopamine
{

namespace services
{

// Define Unknown specialization first, since other specializations use it.
template<>
void
Generator
::_dicom_query_to_mongo_query<Generator::Match::Unknown>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    // Default action: convert to string
    builder << field << value.String();
}

template<>
void
Generator
::_dicom_query_to_mongo_query<Generator::Match::SingleValue>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    builder << field << value.String();
}

template<>
void
Generator
::_dicom_query_to_mongo_query<Generator::Match::ListOfUID>(
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
Generator
::_dicom_query_to_mongo_query<Generator::Match::Universal>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    // Universal is not part of the MongoDB query : do nothing
}

template<>
void
Generator
::_dicom_query_to_mongo_query<Generator::Match::WildCard>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    // Convert DICOM regex to PCRE: replace "*" by ".*", "?" by ".",
    // and escape other special PCRE characters (these are :
    // \^$.[]()+{}
    //
    std::string regex = value.String();
    // Escape "\\" first since we're using it to replace "."
    regex = replace(regex, "\\", "\\\\");
    // Escape "." first since we're using it to replace "*"
    regex = replace(regex, ".", "\\.");
    regex = replace(regex, "*", ".*");
    regex = replace(regex, "?", ".");
    // Escape other PCRE metacharacters
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
Generator
::_dicom_query_to_mongo_query<Generator::Match::Range>(
    std::string const & field, std::string const & vr,
    mongo::BSONElement const & value,
    mongo::BSONObjBuilder & builder) const
{
    std::string const range = value.String();
    std::size_t const separator = range.find("-");
    std::string const begin = range.substr(0, separator);
    std::string const end = range.substr(separator+1, std::string::npos);

    mongo::BSONObjBuilder range_object;
    if(!begin.empty())
    {
        range_object << "$gte" << begin;
    }
    if(!end.empty())
    {
        range_object << "$lte" << end;
    }

    builder << field << range_object.obj();
}

template<>
void
Generator
::_dicom_query_to_mongo_query<Generator::Match::MultipleValues>(
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

Generator
::Generator(const std::string &username):
    _username(username), _allow(false)
{
    // Create DataBase Connection
    create_db_connection(this->_connection, this->_db_name);
}

Generator
::~Generator()
{
    // Nothing to do
}

void Generator::cancel()
{
    loggerWarning() << "Function Not implemented: Generator::cancel()";
}

mongo::BSONObj Generator::next()
{
    if (this->_cursor->more())
    {
        return this->_cursor->next();
    }

    return mongo::BSONObj();
}

bool
Generator
::is_allow() const
{
    return this->_allow;
}

Generator::Match::Type
Generator
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
        else if(element.type() == mongo::Array && vr == "SQ")
        {
            // C.2.2.2.6 Sequence Matching
            type = Match::Sequence;
        }
        else if(element.type() == mongo::Array && vr != "SQ")
        {
            type = Match::MultipleValues;
        }
    }

    return type;
}

Generator::DicomQueryToMongoQuery
Generator
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
    else if(match_type == Match::Range)
    {
        function = &Self::_dicom_query_to_mongo_query<Match::Range>;
    }
//    else if(match_type == Match::Sequence)
//    {
//        function = &Self::_dicom_query_to_mongo_query<Match::Sequence>;
//    }
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

} // namespace services

} // namespace dopamine
