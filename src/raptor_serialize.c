/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * raptor_serialize.c - Serializers
 *
 * Copyright (C) 2004-2006, David Beckett http://purl.org/net/dajobe/
 * Copyright (C) 2004-2005, University of Bristol, UK http://www.bristol.ac.uk/
 * 
 * This package is Free Software and part of Redland http://librdf.org/
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 */


#ifdef HAVE_CONFIG_H
#include <raptor_config.h>
#endif

#ifdef WIN32
#include <win32_raptor_config.h>
#endif


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* Raptor includes */
#include "raptor.h"
#include "raptor_internal.h"


/* prototypes for helper functions */
static raptor_serializer_factory* raptor_get_serializer_factory(const char *name);


/* statics */

/* list of serializer factories */
static raptor_sequence* serializers=NULL;


/* helper methods */

static void
raptor_free_serializer_factory(raptor_serializer_factory* factory)
{
  if(factory->finish_factory)
    factory->finish_factory(factory);
  
  RAPTOR_FREE(raptor_serializer_factory, (void*)factory->name);
  RAPTOR_FREE(raptor_serializer_factory, (void*)factory->label);
  if(factory->alias)
    RAPTOR_FREE(raptor_serializer_factory, (void*)factory->alias);
  if(factory->mime_type)
    RAPTOR_FREE(cstring, factory->mime_type);
  if(factory->uri_string)
    RAPTOR_FREE(raptor_serializer_factory, (void*)factory->uri_string);
  
  RAPTOR_FREE(raptor_serializer_factory, factory);
}


/* class methods */

void
raptor_serializers_init(void)
{
  serializers=raptor_new_sequence((raptor_sequence_free_handler *)raptor_free_serializer_factory, NULL);

  /* raptor_init_serializer_simple(); */

#ifdef RAPTOR_SERIALIZER_NTRIPLES
  raptor_init_serializer_ntriples();
#endif

#ifdef RAPTOR_SERIALIZER_TURTLE
  raptor_init_serializer_turtle();
#endif

#ifdef RAPTOR_SERIALIZER_RDFXML_ABBREV
  raptor_init_serializer_rdfxmla();
#endif

#ifdef RAPTOR_SERIALIZER_RDFXML
  raptor_init_serializer_rdfxml();
#endif

#ifdef RAPTOR_SERIALIZER_RSS_1_0
  raptor_init_serializer_rss10();
#endif

#ifdef RAPTOR_SERIALIZER_ATOM
  raptor_init_serializer_atom();
#endif

#ifdef RAPTOR_SERIALIZER_DOT
  raptor_init_serializer_dot();
#endif
}


/*
 * raptor_serializers_finish - delete all the registered serializers
 */
void
raptor_serializers_finish(void)
{
  raptor_free_sequence(serializers);
  serializers=NULL;
}


/*
 * raptor_serializer_register_factory - Register a syntax that can be generated by a serializer factory
 * @name: the short syntax name
 * @label: readable label for syntax
 * @mime_type: MIME type of the syntax generated by the serializer (or NULL)
 * @uri_string: URI string of the syntax (or NULL)
 * @factory: pointer to function to call to register the factory
 * 
 * INTERNAL
 *
 **/
void
raptor_serializer_register_factory(const char *name, const char *label,
                                   const char *mime_type,
                                   const char *alias,
                                   const unsigned char *uri_string,
                                   void (*factory) (raptor_serializer_factory*)) 
{
  raptor_serializer_factory *serializer;
  char *name_copy, *label_copy, *mime_type_copy, *alias_copy;
  unsigned char *uri_string_copy;
  int i;
  
#if defined(RAPTOR_DEBUG) && RAPTOR_DEBUG > 1
  RAPTOR_DEBUG4("Received registration for syntax serializer %s '%s' with alias '%s'\n", 
                name, label, (alias ? alias : "none"));
  RAPTOR_DEBUG4(raptor_serializer_register_factory,
                "MIME type %s, URI %s\n", 
                (mime_type ? mime_type : "none"),
                (uri_string ? uri_string : "none"));
#endif
  
  for(i=0;
      (serializer=(raptor_serializer_factory*)raptor_sequence_get_at(serializers, i));
      i++) {
    if(!strcmp(serializer->name, name)) {
      RAPTOR_FATAL2("serializer %s already registered\n", name);
      return;
    }
  }
  

  serializer=(raptor_serializer_factory*)RAPTOR_CALLOC(raptor_serializer_factory, 1,
                                               sizeof(raptor_serializer_factory));
  if(!serializer)
    RAPTOR_FATAL1("Out of memory\n");

  name_copy=(char*)RAPTOR_CALLOC(cstring, strlen(name)+1, 1);
  if(!name_copy) {
    RAPTOR_FREE(raptor_serializer, serializer);
    RAPTOR_FATAL1("Out of memory\n");
  }
  strcpy(name_copy, name);
  serializer->name=name_copy;
        
  label_copy=(char*)RAPTOR_CALLOC(cstring, strlen(label)+1, 1);
  if(!label_copy) {
    RAPTOR_FREE(raptor_serializer, serializer);
    RAPTOR_FATAL1("Out of memory\n");
  }
  strcpy(label_copy, label);
  serializer->label=label_copy;

  if(mime_type) {
    mime_type_copy=(char*)RAPTOR_CALLOC(cstring, strlen(mime_type)+1, 1);
    if(!mime_type_copy) {
      RAPTOR_FREE(raptor_serializer, serializer);
      RAPTOR_FATAL1("Out of memory\n");
    }
    strcpy(mime_type_copy, mime_type);
    serializer->mime_type=mime_type_copy;
  }

  if(uri_string) {
    uri_string_copy=(unsigned char*)RAPTOR_CALLOC(cstring, strlen((const char*)uri_string)+1, 1);
    if(!uri_string_copy) {
    RAPTOR_FREE(raptor_serializer, serializer);
    RAPTOR_FATAL1("Out of memory\n");
    }
    strcpy((char*)uri_string_copy, (const char*)uri_string);
    serializer->uri_string=uri_string_copy;
  }
        
  if(alias) {
    alias_copy=(char*)RAPTOR_CALLOC(cstring, strlen(alias)+1, 1);
    if(!alias_copy) {
      RAPTOR_FREE(raptor_serializer, serializer);
      RAPTOR_FATAL1("Out of memory\n");
    }
    strcpy(alias_copy, alias);
    serializer->alias=alias_copy;
  }

  /* Call the serializer registration function on the new object */
  (*factory)(serializer);
  
#if defined(RAPTOR_DEBUG) && RAPTOR_DEBUG > 1
  RAPTOR_DEBUG3("%s has context size %d\n", name, serializer->context_length);
#endif
  
  raptor_sequence_push(serializers, serializer);
}


/**
 * raptor_get_serializer_factory:
 * @name: the factory name or NULL for the default factory
 *
 * Get a serializer factory by name.
 * 
 * Return value: the factory object or NULL if there is no such factory
 **/
static raptor_serializer_factory*
raptor_get_serializer_factory(const char *name) 
{
  raptor_serializer_factory *factory;

  /* return 1st serializer if no particular one wanted - why? */
  if(!name) {
    factory=(raptor_serializer_factory *)raptor_sequence_get_at(serializers, 0);
    if(!factory) {
      RAPTOR_DEBUG1("No (default) serializers registered\n");
      return NULL;
    }
  } else {
    int i;
    
    for(i=0;
        (factory=(raptor_serializer_factory*)raptor_sequence_get_at(serializers, i));
        i++) {
      if(!strcmp(factory->name, name) ||
         (factory->alias && !strcmp(factory->alias, name)))
        break;

    }

    /* else FACTORY name not found */
    if(!factory) {
      RAPTOR_DEBUG2("No serializer with name %s found\n", name);
      return NULL;
    }
  }
        
  return factory;
}


/**
 * raptor_serializers_enumerate:
 * @counter: index into the list of syntaxes
 * @name: pointer to store the name of the syntax (or NULL)
 * @label: pointer to store syntax readable label (or NULL)
 * @mime_type: pointer to store syntax MIME Type (or NULL)
 * @uri_string: pointer to store syntax URI string (or NULL)
 *
 * Get information on syntax serializers.
 * 
 * Return value: non 0 on failure of if counter is out of range
 **/
int
raptor_serializers_enumerate(const unsigned int counter,
                             const char **name, const char **label,
                             const char **mime_type,
                             const unsigned char **uri_string)
{
  raptor_serializer_factory *factory;

  factory=(raptor_serializer_factory*)raptor_sequence_get_at(serializers,
                                                             counter);

  if(!factory)
    return 1;

  if(name)
    *name=factory->name;
  if(label)
    *label=factory->label;
  if(mime_type)
    *mime_type=factory->mime_type;
  if(uri_string)
    *uri_string=factory->uri_string;

  return 0;
}


/**
 * raptor_serializer_syntax_name_check:
 * @name: the syntax name
 *
 * Check name of a serializer.
 *
 * Return value: non 0 if name is a known syntax name
 */
int
raptor_serializer_syntax_name_check(const char *name)
{
  return (raptor_get_serializer_factory(name) != NULL);
}


/**
 * raptor_new_serializer:
 * @name: the serializer name
 *
 * Constructor - create a new raptor_serializer object
 *
 * Return value: a new #raptor_serializer object or NULL on failure
 */
raptor_serializer*
raptor_new_serializer(const char *name)
{
  raptor_serializer_factory* factory;
  raptor_serializer* rdf_serializer;

  factory=raptor_get_serializer_factory(name);
  if(!factory)
    return NULL;

  rdf_serializer=(raptor_serializer*)RAPTOR_CALLOC(raptor_serializer, 1,
                                           sizeof(raptor_serializer));
  if(!rdf_serializer)
    return NULL;
  
  rdf_serializer->context=(char*)RAPTOR_CALLOC(raptor_serializer_context, 1,
                                               factory->context_length);
  if(!rdf_serializer->context) {
    raptor_free_serializer(rdf_serializer);
    return NULL;
  }
  
  rdf_serializer->factory=factory;

  /* Default features */
  /* Emit relative URIs where possible */
  rdf_serializer->feature_relative_uris=1;

  rdf_serializer->feature_resource_border  =
    rdf_serializer->feature_literal_border =
    rdf_serializer->feature_bnode_border   =
    rdf_serializer->feature_resource_fill  =
    rdf_serializer->feature_literal_fill   =
    rdf_serializer->feature_bnode_fill     = NULL;

  /* XML 1.0 output */
  rdf_serializer->xml_version=10;

  /* Write XML declaration */
  rdf_serializer->feature_write_xml_declaration=1;

  if(factory->init(rdf_serializer, name)) {
    raptor_free_serializer(rdf_serializer);
    return NULL;
  }
  
  return rdf_serializer;
}


/**
 * raptor_serialize_start:
 * @rdf_serializer:  the #raptor_serializer
 * @uri: base URI or NULL if no base URI is required
 * @iostream: #raptor_iostream to write serialization to
 * 
 * Start serialization with given base URI
 *
 * Return value: non-0 on failure.
 **/
int
raptor_serialize_start(raptor_serializer *rdf_serializer, raptor_uri *uri,
                       raptor_iostream *iostream) 
{
  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  if(!iostream)
    return 1;
  
  if(uri)
    uri=raptor_uri_copy(uri);
  
  rdf_serializer->base_uri=uri;
  rdf_serializer->locator.uri=uri;
  rdf_serializer->locator.line=rdf_serializer->locator.column = 0;

  rdf_serializer->iostream=iostream;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}


/**
 * raptor_serialize_start_to_filename:
 * @rdf_serializer:  the #raptor_serializer
 * @filename:  filename to serialize to
 *
 * Start serializing to a filename.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serialize_start_to_filename(raptor_serializer *rdf_serializer,
                                   const char *filename)
{
  unsigned char *uri_string=raptor_uri_filename_to_uri_string(filename);
  if(!uri_string)
    return 1;

  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  rdf_serializer->base_uri=raptor_new_uri(uri_string);
  rdf_serializer->locator.uri=rdf_serializer->base_uri;
  rdf_serializer->locator.line=rdf_serializer->locator.column = 0;

  RAPTOR_FREE(cstring, uri_string);

  rdf_serializer->iostream=raptor_new_iostream_to_filename(filename);
  if(!rdf_serializer->iostream)
    return 1;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}



/**
 * raptor_serialize_start_to_string:
 * @rdf_serializer:  the #raptor_serializer
 * @uri: base URI or NULL if no base URI is required
 * @string_p: pointer to location to hold string
 * @length_p: pointer to location to hold length of string (or NULL)
 *
 * Start serializing to a string.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serialize_start_to_string(raptor_serializer *rdf_serializer,
                                 raptor_uri *uri,
                                 void **string_p, size_t *length_p) 
{
  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  if(uri)
    rdf_serializer->base_uri=raptor_uri_copy(uri);
  else
    rdf_serializer->base_uri=NULL;
  rdf_serializer->locator.uri=rdf_serializer->base_uri;
  rdf_serializer->locator.line=rdf_serializer->locator.column = 0;


  rdf_serializer->iostream=raptor_new_iostream_to_string(string_p, length_p, 
                                                         NULL);
  if(!rdf_serializer->iostream)
    return 1;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}


/**
 * raptor_serialize_start_to_file_handle:
 * @rdf_serializer:  the #raptor_serializer
 * @uri: base URI or NULL if no base URI is required
 * @fh:  FILE* to serialize to
 *
 * Start serializing to a FILE*.
 * 
 * NOTE: This does not fclose the handle when it is finished.
8
 * Return value: non-0 on failure.
 **/
int
raptor_serialize_start_to_file_handle(raptor_serializer *rdf_serializer,
                                      raptor_uri *uri, FILE *fh) 
{
  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  if(uri)
    rdf_serializer->base_uri=raptor_uri_copy(uri);
  else
    rdf_serializer->base_uri=NULL;
  rdf_serializer->locator.uri=rdf_serializer->base_uri;
  rdf_serializer->locator.line=rdf_serializer->locator.column = 0;

  rdf_serializer->iostream=raptor_new_iostream_to_file_handle(fh);
  if(!rdf_serializer->iostream)
    return 1;

  if(rdf_serializer->factory->serialize_start)
    return rdf_serializer->factory->serialize_start(rdf_serializer);
  return 0;
}


/**
 * raptor_serialize_set_namespace:
 * @rdf_serializer: the #raptor_serializer
 * @uri: #raptor_uri of namespace
 * @prefix: prefix to use
 *
 * set a namespace uri/prefix mapping for serializing.
 *
 * return value: non-0 on failure.
 **/
int
raptor_serialize_set_namespace(raptor_serializer* rdf_serializer,
                               raptor_uri *uri, const unsigned char *prefix) 
{
  if(rdf_serializer->factory->declare_namespace)
    return rdf_serializer->factory->declare_namespace(rdf_serializer, 
                                                      uri, prefix);

  return 1;
}


/**
 * raptor_serialize_set_namespace_from_namespace:
 * @rdf_serializer: the #raptor_serializer
 * @nspace: #raptor_namespace to set
 *
 * Set a namespace uri/prefix mapping for serializing from an existing namespace.
 *
 * Return value: non-0 on failure.
 **/
int
raptor_serialize_set_namespace_from_namespace(raptor_serializer* rdf_serializer,
                                              raptor_namespace *nspace)
{
  if(rdf_serializer->factory->declare_namespace_from_namespace)
    return rdf_serializer->factory->declare_namespace_from_namespace(rdf_serializer, 
                                                                     nspace);
  else if (rdf_serializer->factory->declare_namespace)
    return rdf_serializer->factory->declare_namespace(rdf_serializer, 
                                                      raptor_namespace_get_uri(nspace),
                                                      raptor_namespace_get_prefix(nspace));

  return 1;
}


/**
 * raptor_serialize_statement:
 * @rdf_serializer: the #raptor_serializer
 * @statement: #raptor_statement to serialize to a syntax
 *
 * Serialize a statement.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serialize_statement(raptor_serializer* rdf_serializer,
                           const raptor_statement *statement)
{
  if(!rdf_serializer->iostream)
    return 1;
  return rdf_serializer->factory->serialize_statement(rdf_serializer, statement);
}


/**
 * raptor_serialize_end:
 * @rdf_serializer:  the #raptor_serializer
 *
 * End a serialization.
 * 
 * Return value: non-0 on failure.
 **/
int
raptor_serialize_end(raptor_serializer *rdf_serializer) 
{
  int rc;
  
  if(!rdf_serializer->iostream)
    return 1;

  if(rdf_serializer->factory->serialize_end)
    rc=rdf_serializer->factory->serialize_end(rdf_serializer);
  else
    rc=0;

  if(rdf_serializer->iostream) {
    raptor_free_iostream(rdf_serializer->iostream);
    rdf_serializer->iostream=NULL;
  }
  return rc;
}



/**
 * raptor_free_serializer:
 * @rdf_serializer: #raptor_serializer object
 *
 * Destructor - destroy a raptor_serializer object.
 * 
 **/
void
raptor_free_serializer(raptor_serializer* rdf_serializer) 
{
  if(rdf_serializer->factory)
    rdf_serializer->factory->terminate(rdf_serializer);

  if(rdf_serializer->context)
    RAPTOR_FREE(raptor_serializer_context, rdf_serializer->context);

  if(rdf_serializer->base_uri)
    raptor_free_uri(rdf_serializer->base_uri);

  if(rdf_serializer->feature_start_uri)
    raptor_free_uri(rdf_serializer->feature_start_uri);

  RAPTOR_FREE(raptor_serializer, rdf_serializer);
}


/**
 * raptor_serializer_get_iostream:
 * @serializer: #raptor_serializer object
 *
 * Get the current serializer iostream.
 *
 * Return value: the serializer's current iostream or NULL if 
 **/
raptor_iostream*
raptor_serializer_get_iostream(raptor_serializer *serializer)
{
  return serializer->iostream;
}



/**
 * raptor_serializer_features_enumerate:
 * @feature: feature enumeration (0+)
 * @name: pointer to store feature short name (or NULL)
 * @uri: pointer to store feature URI (or NULL)
 * @label: pointer to feature label (or NULL)
 *
 * Get list of serializer features.
 * 
 * If uri is not NULL, a pointer toa new raptor_uri is returned
 * that must be freed by the caller with raptor_free_uri().
 *
 * Return value: 0 on success, <0 on failure, >0 if feature is unknown
 **/
int
raptor_serializer_features_enumerate(const raptor_feature feature,
                                     const char **name, 
                                     raptor_uri **uri, const char **label)
{
  return raptor_features_enumerate_common(feature, name, uri, label, 2);
}


/**
 * raptor_serializer_set_feature:
 * @serializer: #raptor_serializer serializer object
 * @feature: feature to set from enumerated #raptor_feature values
 * @value: integer feature value (0 or larger)
 *
 * Set serializer features with integer values.
 * 
 * The allowed features are available via raptor_features_enumerate().
 *
 * Return value: non 0 on failure or if the feature is unknown
 **/
int
raptor_serializer_set_feature(raptor_serializer *serializer, 
                              raptor_feature feature, int value)
{
  if(value < 0)
    return -1;
  
  switch(feature) {
    case RAPTOR_FEATURE_RELATIVE_URIS:
      serializer->feature_relative_uris=value;
      break;

    case RAPTOR_FEATURE_START_URI:
      return -1;
      break;

    case RAPTOR_FEATURE_WRITER_XML_VERSION:
      if(value == 10 || value == 11)
        serializer->xml_version=value;
      break;

    case RAPTOR_FEATURE_WRITER_XML_DECLARATION:
      serializer->feature_write_xml_declaration = value;
      break;

    /* parser features */
    case RAPTOR_FEATURE_SCANNING:
    case RAPTOR_FEATURE_ASSUME_IS_RDF:
    case RAPTOR_FEATURE_ALLOW_NON_NS_ATTRIBUTES:
    case RAPTOR_FEATURE_ALLOW_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_ALLOW_BAGID:
    case RAPTOR_FEATURE_ALLOW_RDF_TYPE_RDF_LIST:
    case RAPTOR_FEATURE_NORMALIZE_LANGUAGE:
    case RAPTOR_FEATURE_NON_NFC_FATAL:
    case RAPTOR_FEATURE_WARN_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_CHECK_RDF_ID:
    case RAPTOR_FEATURE_HTML_TAG_SOUP:

    /* Shared */
    case RAPTOR_FEATURE_NO_NET:

    /* XML writer features */
    case RAPTOR_FEATURE_WRITER_AUTO_INDENT:
    case RAPTOR_FEATURE_WRITER_AUTO_EMPTY:
    case RAPTOR_FEATURE_WRITER_INDENT_WIDTH:

    /* String features */
    case RAPTOR_FEATURE_RESOURCE_BORDER:
    case RAPTOR_FEATURE_LITERAL_BORDER:
    case RAPTOR_FEATURE_BNODE_BORDER:
    case RAPTOR_FEATURE_RESOURCE_FILL:
    case RAPTOR_FEATURE_LITERAL_FILL:
    case RAPTOR_FEATURE_BNODE_FILL:

    default:
      return -1;
      break;
  }

  return 0;
}


static int
raptor_serializer_copy_string(unsigned char ** dest,
			      const unsigned char * src)
{
  size_t src_len=strlen((const char *)src);

  if(*dest) {
    RAPTOR_FREE(cstring, *dest);
    *dest=NULL;
  }

  if(!(*dest=(unsigned char*)RAPTOR_MALLOC(cstring, src_len+1)))
    return -1;

  strcpy((char *)(*dest), (const char *)src);

  return 0;
}


/**
 * raptor_serializer_set_feature_string:
 * @serializer: #raptor_serializer serializer object
 * @feature: feature to set from enumerated #raptor_feature values
 * @value: feature value
 *
 * Set serializer features with string values.
 * 
 * The allowed features are available via raptor_serializer_features_enumerate().
 * If the feature type is integer, the value is interpreted as an integer.
 *
 * Return value: non 0 on failure or if the feature is unknown
 **/
int
raptor_serializer_set_feature_string(raptor_serializer *serializer, 
                                     raptor_feature feature, 
                                     const unsigned char *value)
{
  int value_is_string=(raptor_feature_value_type(feature) == 1);
  if(!value_is_string)
    return raptor_serializer_set_feature(serializer, feature, 
                                         atoi((const char*)value));

  switch(feature) {
    case RAPTOR_FEATURE_START_URI:
      if(value)
        serializer->feature_start_uri=raptor_new_uri(value);
      else
        return -1;
      break;

    case RAPTOR_FEATURE_RELATIVE_URIS:
      /* actually handled above because value_is_string is false */
      return -1;
      break;

    /* parser features */
    case RAPTOR_FEATURE_SCANNING:
    case RAPTOR_FEATURE_ASSUME_IS_RDF:
    case RAPTOR_FEATURE_ALLOW_NON_NS_ATTRIBUTES:
    case RAPTOR_FEATURE_ALLOW_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_ALLOW_BAGID:
    case RAPTOR_FEATURE_ALLOW_RDF_TYPE_RDF_LIST:
    case RAPTOR_FEATURE_NORMALIZE_LANGUAGE:
    case RAPTOR_FEATURE_NON_NFC_FATAL:
    case RAPTOR_FEATURE_WARN_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_CHECK_RDF_ID:
    case RAPTOR_FEATURE_HTML_TAG_SOUP:

    /* Shared */
    case RAPTOR_FEATURE_NO_NET:

    /* XML writer features */
    case RAPTOR_FEATURE_WRITER_AUTO_INDENT:
    case RAPTOR_FEATURE_WRITER_AUTO_EMPTY:
    case RAPTOR_FEATURE_WRITER_INDENT_WIDTH:
    case RAPTOR_FEATURE_WRITER_XML_VERSION:
    case RAPTOR_FEATURE_WRITER_XML_DECLARATION:

    /* GraphViz serializer features */
    case RAPTOR_FEATURE_RESOURCE_BORDER:
      return raptor_serializer_copy_string(
        (unsigned char **)&(serializer->feature_resource_border), value);
      break;
    case RAPTOR_FEATURE_LITERAL_BORDER:
      return raptor_serializer_copy_string(
        (unsigned char **)&(serializer->feature_literal_border), value);
      break;
    case RAPTOR_FEATURE_BNODE_BORDER:
      return raptor_serializer_copy_string(
        (unsigned char **)&(serializer->feature_bnode_border), value);
      break;
    case RAPTOR_FEATURE_RESOURCE_FILL:
      return raptor_serializer_copy_string(
        (unsigned char **)&(serializer->feature_resource_fill), value);
      break;
    case RAPTOR_FEATURE_LITERAL_FILL:
      return raptor_serializer_copy_string(
        (unsigned char **)&(serializer->feature_literal_fill), value);
      break;
    case RAPTOR_FEATURE_BNODE_FILL:
      return raptor_serializer_copy_string(
        (unsigned char **)&(serializer->feature_bnode_fill), value);
      break;

    default:
      return -1;
      break;
  }

  return 0;
}


/**
 * raptor_serializer_get_feature:
 * @serializer: #raptor_serializer serializer object
 * @feature: feature to get value
 *
 * Get various serializer features.
 * 
 * The allowed features are available via raptor_features_enumerate().
 *
 * Note: no feature value is negative
 *
 * Return value: feature value or < 0 for an illegal feature
 **/
int
raptor_serializer_get_feature(raptor_serializer *serializer, 
                              raptor_feature feature)
{
  int result= -1;
  
  switch(feature) {
    case RAPTOR_FEATURE_RELATIVE_URIS:
      result=(serializer->feature_relative_uris != 0);
      break;

    /* String features */
    case RAPTOR_FEATURE_START_URI:
    case RAPTOR_FEATURE_RESOURCE_BORDER:
    case RAPTOR_FEATURE_LITERAL_BORDER:
    case RAPTOR_FEATURE_BNODE_BORDER:
    case RAPTOR_FEATURE_RESOURCE_FILL:
    case RAPTOR_FEATURE_LITERAL_FILL:
    case RAPTOR_FEATURE_BNODE_FILL:
      result= -1;
      break;

    case RAPTOR_FEATURE_WRITER_XML_VERSION:
      result=serializer->xml_version;
      break;
  
    case RAPTOR_FEATURE_WRITER_XML_DECLARATION:
      result=serializer->feature_write_xml_declaration;
      break;
      
    /* parser features */
    case RAPTOR_FEATURE_SCANNING:
    case RAPTOR_FEATURE_ASSUME_IS_RDF:
    case RAPTOR_FEATURE_ALLOW_NON_NS_ATTRIBUTES:
    case RAPTOR_FEATURE_ALLOW_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_ALLOW_BAGID:
    case RAPTOR_FEATURE_ALLOW_RDF_TYPE_RDF_LIST:
    case RAPTOR_FEATURE_NORMALIZE_LANGUAGE:
    case RAPTOR_FEATURE_NON_NFC_FATAL:
    case RAPTOR_FEATURE_WARN_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_CHECK_RDF_ID:
    case RAPTOR_FEATURE_HTML_TAG_SOUP:

    /* Shared */
    case RAPTOR_FEATURE_NO_NET:

    /* XML writer features */
    case RAPTOR_FEATURE_WRITER_AUTO_INDENT:
    case RAPTOR_FEATURE_WRITER_AUTO_EMPTY:
    case RAPTOR_FEATURE_WRITER_INDENT_WIDTH:

    default:
      break;
  }
  
  return result;
}


/**
 * raptor_serializer_get_feature_string:
 * @serializer: #raptor_serializer serializer object
 * @feature: feature to get value
 *
 * Get serializer features with string values.
 * 
 * The allowed features are available via raptor_features_enumerate().
 *
 * Return value: feature value or NULL for an illegal feature or no value
 **/
const unsigned char *
raptor_serializer_get_feature_string(raptor_serializer *serializer, 
                                     raptor_feature feature)
{
  int value_is_string=(raptor_feature_value_type(feature) == 1);
  if(!value_is_string)
    return NULL;
  
  switch(feature) {
    case RAPTOR_FEATURE_START_URI:
      if(serializer->feature_start_uri)
        return raptor_uri_to_string(serializer->feature_start_uri);
      break;

    case RAPTOR_FEATURE_RELATIVE_URIS:
      /* actually handled above because value_is_string is false */
      return NULL;
      break;
      
    /* GraphViz serializer features */
    case RAPTOR_FEATURE_RESOURCE_BORDER:
      return (unsigned char *)(serializer->feature_resource_border);
      break;
    case RAPTOR_FEATURE_LITERAL_BORDER:
      return (unsigned char *)(serializer->feature_literal_border);
      break;
    case RAPTOR_FEATURE_BNODE_BORDER:
      return (unsigned char *)(serializer->feature_bnode_border);
      break;
    case RAPTOR_FEATURE_RESOURCE_FILL:
      return (unsigned char *)(serializer->feature_resource_fill);
      break;
    case RAPTOR_FEATURE_LITERAL_FILL:
      return (unsigned char *)(serializer->feature_literal_fill);
      break;
    case RAPTOR_FEATURE_BNODE_FILL:
      return (unsigned char *)(serializer->feature_bnode_fill);
      break;
        
    /* parser features */
    case RAPTOR_FEATURE_SCANNING:
    case RAPTOR_FEATURE_ASSUME_IS_RDF:
    case RAPTOR_FEATURE_ALLOW_NON_NS_ATTRIBUTES:
    case RAPTOR_FEATURE_ALLOW_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_ALLOW_BAGID:
    case RAPTOR_FEATURE_ALLOW_RDF_TYPE_RDF_LIST:
    case RAPTOR_FEATURE_NORMALIZE_LANGUAGE:
    case RAPTOR_FEATURE_NON_NFC_FATAL:
    case RAPTOR_FEATURE_WARN_OTHER_PARSETYPES:
    case RAPTOR_FEATURE_CHECK_RDF_ID:
    case RAPTOR_FEATURE_HTML_TAG_SOUP:

    /* Shared */
    case RAPTOR_FEATURE_NO_NET:

    /* XML writer features */
    case RAPTOR_FEATURE_WRITER_AUTO_INDENT:
    case RAPTOR_FEATURE_WRITER_AUTO_EMPTY:
    case RAPTOR_FEATURE_WRITER_INDENT_WIDTH:
    case RAPTOR_FEATURE_WRITER_XML_VERSION:
    case RAPTOR_FEATURE_WRITER_XML_DECLARATION:

    default:
      return NULL;
      break;
  }

  return NULL;
}


/*
 * raptor_serializer_error - Error from a serializer - Internal
 */
void
raptor_serializer_error(raptor_serializer* serializer, const char *message, ...)
{
  va_list arguments;

  va_start(arguments, message);

  raptor_serializer_error_varargs(serializer, message, arguments);
  
  va_end(arguments);
}


/*
 * raptor_serializer_simple_error - Error from a serializer - Internal
 *
 * Matches the raptor_simple_message_handler API but same as
 * raptor_serializer_error 
 */
void
raptor_serializer_simple_error(void* serializer, const char *message, ...)
{
  va_list arguments;

  va_start(arguments, message);

  raptor_serializer_error_varargs((raptor_serializer*)serializer, message, arguments);
  
  va_end(arguments);
}


/*
 * raptor_serializer_error_varargs - Error from a serializer - Internal
 */
void
raptor_serializer_error_varargs(raptor_serializer* serializer, 
                                const char *message, 
                                va_list arguments)
{
  if(serializer->error_handler) {
    char *buffer=raptor_vsnprintf(message, arguments);
    size_t length;
    if(!buffer) {
      fprintf(stderr, "raptor_serializer_error_varargs: Out of memory\n");
      return;
    }
    length=strlen(buffer);
    if(buffer[length-1]=='\n')
      buffer[length-1]='\0';
    serializer->error_handler(serializer->error_user_data, 
                              &serializer->locator, buffer);
    RAPTOR_FREE(cstring, buffer);
    return;
  }

  raptor_print_locator(stderr, &serializer->locator);
  fprintf(stderr, " raptor error - ");
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);
}


/*
 * raptor_serializer_warning - Warning from a serializer - Internal
 */
void
raptor_serializer_warning(raptor_serializer* serializer, const char *message, ...)
{
  va_list arguments;

  va_start(arguments, message);

  raptor_serializer_warning_varargs(serializer, message, arguments);

  va_end(arguments);
}


/*
 * raptor_serializer_warning - Warning from a serializer - Internal
 */
void
raptor_serializer_warning_varargs(raptor_serializer* serializer, const char *message, 
                                  va_list arguments)
{

  if(serializer->warning_handler) {
    char *buffer=raptor_vsnprintf(message, arguments);
    size_t length;
    if(!buffer) {
      fprintf(stderr, "raptor_serializer_warning_varargs: Out of memory\n");
      return;
    }
    length=strlen(buffer);
    if(buffer[length-1]=='\n')
      buffer[length-1]='\0';
    serializer->warning_handler(serializer->warning_user_data,
                                &serializer->locator, buffer);
    RAPTOR_FREE(cstring, buffer);
    return;
  }

  raptor_print_locator(stderr, &serializer->locator);
  fprintf(stderr, " raptor warning - ");
  vfprintf(stderr, message, arguments);
  fputc('\n', stderr);
}


/**
 * raptor_serializer_set_error_handler:
 * @serializer: the serializer
 * @user_data: user data to pass to function
 * @handler: pointer to the function
 *
 * Set the serializer error handling function.
 * 
 * The function will receive callbacks when the serializer fails.
 * 
 **/
void
raptor_serializer_set_error_handler(raptor_serializer* serializer, 
                                    void *user_data,
                                    raptor_message_handler handler)
{
  serializer->error_user_data=user_data;
  serializer->error_handler=handler;
}


/**
 * raptor_serializer_set_warning_handler:
 * @serializer: the serializer
 * @user_data: user data to pass to function
 * @handler: pointer to the function
 *
 * Set the serializer warning handling function.
 * 
 * The function will receive callbacks when the serializer fails.
 * 
 **/
void
raptor_serializer_set_warning_handler(raptor_serializer* serializer, 
                                      void *user_data,
                                      raptor_message_handler handler)
{
  serializer->warning_user_data=user_data;
  serializer->warning_handler=handler;
}


/**
 * raptor_serializer_get_locator:
 * @rdf_serializer: raptor serializer
 *
 * Get the serializer raptor locator object.
 * 
 * Return value: raptor locator
 **/
raptor_locator*
raptor_serializer_get_locator(raptor_serializer *rdf_serializer)
{
  return &rdf_serializer->locator;
}
