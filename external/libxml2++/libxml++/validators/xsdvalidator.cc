/* Copyright (C) 2014 The libxml++ development team
 *
 * This file is part of libxml++.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "libxml++/validators/xsdvalidator.h"
#include "libxml++/xsdschema.h"

#include <libxml/xmlschemas.h>

namespace xmlpp
{

struct XsdValidator::Impl
{
  Impl() : schema(0), is_schema_owner(false), context(0) {}

  XsdSchema* schema;
  bool is_schema_owner;
  _xmlSchemaValidCtxt* context;
};


XsdValidator::XsdValidator()
: pimpl_(new Impl)
{
}

XsdValidator::XsdValidator(const std::string& filename)
: pimpl_(new Impl)
{
  parse_file(filename);
}

XsdValidator::XsdValidator(const Document* document)
: pimpl_(new Impl)
{
  parse_document(document);
}

XsdValidator::XsdValidator(XsdSchema* schema, bool take_ownership)
: pimpl_(new Impl)
{
  set_schema(schema, take_ownership);
}

XsdValidator::~XsdValidator()
{
  release_underlying();
}

void XsdValidator::parse_file(const std::string& filename)
{
  set_schema(new XsdSchema(filename), true);
}

void XsdValidator::parse_memory(const std::string& contents)
{
  std::auto_ptr<XsdSchema> schema(new XsdSchema());
  schema->parse_memory(contents);
  set_schema(schema.release(), true);
}

void XsdValidator::parse_document(const Document* document)
{
  set_schema(new XsdSchema(document), true);
}

void XsdValidator::set_schema(XsdSchema* schema, bool take_ownership)
{
  release_underlying();
  pimpl_->schema = schema;
  pimpl_->is_schema_owner = take_ownership;
}

void XsdValidator::release_underlying()
{
  if (pimpl_->context)
  {
    xmlSchemaFreeValidCtxt(pimpl_->context);
    pimpl_->context = 0;
  }

  if (pimpl_->schema)
  {
    if (pimpl_->is_schema_owner)
      delete pimpl_->schema;
    pimpl_->schema = 0;
  }

  SchemaValidatorBase::release_underlying();
}

XsdSchema* XsdValidator::get_schema()
{
  return pimpl_->schema;
}

const XsdSchema* XsdValidator::get_schema() const
{
  return pimpl_->schema;
}

XsdValidator::operator const void*() const
{
  return reinterpret_cast<const void*>(pimpl_->schema && pimpl_->schema->cobj());
}


void XsdValidator::initialize_valid()
{
  xmlSchemaSetValidErrors(pimpl_->context, &callback_validity_error, &callback_validity_warning, this);
  SchemaValidatorBase::initialize_valid();
}


void XsdValidator::validate(const Document* document)
{
  if (!document)
    throw internal_error("XsdValidator::validate(): document must not be 0.");

  if (!*this)
    throw internal_error("XsdValidator::validate(): Must have a schema to validate document");

  // A context is required at this stage only
  if (!pimpl_->context)
    pimpl_->context = xmlSchemaNewValidCtxt(pimpl_->schema->cobj());

  if (!pimpl_->context)
    throw internal_error("XsdValidator::validate(): Could not create validating context");

  xmlResetLastError();
  initialize_valid();

  const int res = xmlSchemaValidateDoc(pimpl_->context, const_cast<xmlDoc*>(document->cobj()));
  if (res != 0)
  {
    check_for_exception();

    std::string error_str = format_xml_error();
    if (error_str.empty())
      error_str = "Error code from xmlSchemaValidateDoc(): " + std::to_string(res);
    throw validity_error("Document failed XSD schema validation.\n" + error_str);
  }
}

void XsdValidator::validate(const std::string& filename)
{
  if (!*this)
    throw internal_error("XsdValidator::validate(): Must have a schema to validate file.");

  // A context is required at this stage only
  if (!pimpl_->context)
    pimpl_->context = xmlSchemaNewValidCtxt(pimpl_->schema->cobj());

  if (!pimpl_->context)
    throw internal_error("XsdValidator::validate(): Could not create validating context");

  xmlResetLastError();
  initialize_valid();

  const int res = xmlSchemaValidateFile(pimpl_->context, filename.c_str(), 0);
  if (res != 0)
  {
    check_for_exception();

    std::string error_str = format_xml_error();
    if (error_str.empty())
      error_str = "Error code from xmlSchemaValidateFile(): " + std::to_string(res);
    throw validity_error("XML file failed XSD schema validation.\n" + error_str);
  }
}

} // namespace xmlpp
