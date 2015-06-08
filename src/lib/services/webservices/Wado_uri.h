/*************************************************************************
 * dopamine - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#ifndef _5df462f6_0976_4a99_b597_0761a549979c
#define _5df462f6_0976_4a99_b597_0761a549979c

#include <map>

#include "Wado.h"

namespace dopamine
{

namespace services
{

struct parameters
{
    bool _mandatory;
    bool _used;

    parameters(bool mandatory, bool used):
        _mandatory(mandatory), _used(used) {}
};

// Mandatory Request Parameters
const std::string REQUEST_TYPE = "requestType";
const std::string STUDY_INSTANCE_UID = "studyUID";
const std::string SERIES_INSTANCE_UID = "seriesUID";
const std::string SOP_INSTANCE_UID = "objectUID";

// List of Request Parameters: see PS3.17 Table HHH.1-1
const std::map<std::string, parameters> RequestParameters = {
    { REQUEST_TYPE, parameters(true, true) },
    { STUDY_INSTANCE_UID, parameters(true, true) },
    { SERIES_INSTANCE_UID, parameters(true, true) },
    { SOP_INSTANCE_UID, parameters(true, true) },
    { "contentType", parameters(false, false) },
    { "charset", parameters(false, false) },
    { "anonymize", parameters(false, false) },
    { "annotation", parameters(false, false) },
    { "Rows", parameters(false, false) },
    { "Column", parameters(false, false) },
    { "region", parameters(false, false) },
    { "windowCenter", parameters(false, false) },
    { "windowWidth", parameters(false, false) },
    { "imageQuality", parameters(false, false) },
    { "presentationUID", parameters(false, false) },
    { "presentationSeriesUID", parameters(false, false) },
    { "transferSyntax", parameters(false, false) },
    { "frameNumber", parameters(false, false) }
};

class Wado_uri : public Wado
{
public:
    Wado_uri(std::string const & querystring,
             std::string const & remoteuser = "");

    ~Wado_uri();

    std::string get_filename() const;

protected:

private:
    virtual mongo::BSONObj parse_string();

    std::string _filename;

};

} // namespace services

} // namespace dopamine

#endif // _5df462f6_0976_4a99_b597_0761a549979c
