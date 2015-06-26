[![Build Status](https://travis-ci.org/google/primarysources.svg?branch=master)](https://travis-ci.org/google/primarysources)

# REST API

## Get Entity by QID

The following HTTP request retrieves an entity by Wikidata QID:

    GET /entities/<QID>?dataset=<DATASET>
      
The service is modeled after the Wikidata REST API, and supports retrieving data in JSON and TSV format
(see content negotiation below). `dataset` is an optional parameter allowing to only retrieve statements
from a specific dataset.
    
Status Codes:

  * 200: entity found, returned as JSON
  * 404: entity not found
  * 500: server error


## Get Random Entity (by Topic)

The following HTTP request retrieves a random entity, optionally by topic. The selection procedure is
left to the backend and might be improved in the future based on user profile etc.

    GET /entities/any?topic=<TOPIC>&user=<WikidataUser>
      
Both topic and user are optional, in which case the backend selects a random unapproved entity.
    
Status Codes:

  * 200: entity found, returned as JSON
  * 404: entity not found
  * 500: server error


## Get Statement by Database ID

The following HTTP request retrieves a single Wikidata statement by its internal database ID:

    GET /statements/<ID>

The statement can be returned in any of the content negotiation formats (see below).


## Get Random Statements

The following HTTP request retrieves a random list of non-approved statements. The
selection procedure is left to the backend.

    GET /statements/any

The statement can be returned in any of the content negotiation formats (see below).


## Mark Statement as Approved (Wrong, Othersource)

The following HTTP request marks a statement identified by a Wikidata QID as approved:

    POST /statements/<ID>?state=<STATE>&user=<WikidataUser>
    
where <STATE> can be one of "approved", "wrong", "duplicate", "blacklisted", or "othersource". 
The WikidataUser passed as argument is used for tracking purposes only and stored in the 
database together with the approval flag.
   
Status Codes:

  * 200: statement found and marked as approved by user <WikidataUser>
  * 404: statement not found
  * 409: statement found but was already marked as approved by another user
  * 500: server error
  
## Import Statements

The backend provides a REST service to import sequence of statements in Wikidata TSV format 
into the database. The service reads the data from the raw request POST body. Data may 
optionally be gzipped for better memory usage.

Request:

    POST /import?token=<token>&gzip=<true|false>

The token is a kind of password configurable in config.json that is used as a very simple 
authentication mechanism. It is recommended to protect this service also on the webserver 
level (e.g. using HTTP authentication or IP-based access control).

Status Codes:
   * 200: request successful, statements imported; returns number of imported statements as JSON
   * 401: in case the token does not match the configured import token
   * 500: import failed (e.g. parse error)
  
## Delete Statements

The delete service allows to delete all statements with a specified state from the database.

Request:

    POST /delete?state=<state>&token=<token>

The state is one of "approved", "wrong", "othersource", "blacklisted", "duplicate". The token is a 
kind of password configurable in config.json that is used as a very simple authentication mechanism. 
It is recommended to protect this service also on the webserver level (e.g. using HTTP authentication 
or IP-based access control).

Status Codes:
   * 200: request successful, statements deleted
   * 401: in case the token does not match the configured delete token
   * 500: delete failed

## Status

The status service returns a JSON object containing current status information like total number 
of statements, number of approved statements, top users, system information, etc.

Request:

     GET /status

Status Codes:
   * 200: request successful, status returned as JSON object
   * 500: server error

## Content Negotiation
  
GET requests to the backend webservices currently support 3 different serialization formats that
can be selected by setting appropriate `Accept:` headers:
 
  * [Wikidata Tab Separated](http://tools.wmflabs.org/wikidata-todo/quick_statements.php) using header
    `text/vnd.wikidata+tsv`
  * [Wikidata JSON](https://www.mediawiki.org/wiki/Wikibase/Notes/JSON) using header
    `application/vnd.wikidata+json`
  * Envelope JSON, wrapping Wikidata TSV in a JSON envelope containing the database ID and version
    information; this is the default format.
 
# Building and Installation

The REST application is implemented in C++ using the [CppCMS](http://cppcms.com/) 
framework and its [CppDB](http://cppcms.com/sql/cppdb/) companion. Building the
application requires that you first download and install both frameworks and 
their dependencies (follow instructions on CppCMS webpage). In particular,
you need to install sqlite3 with development headers.

## Build from Source

The sources tool uses cmake for building the server. Please use the following commands to create the binary.

    $ mkdir build && cd build
    $ cmake ..
    $ make
    
## Initialise and Fill Database
    
The current development version of the backend uses sqlite3 as database engine. 
Initialise the database schema by running
    
    $ sqlite3 fb.db < schema.sqlite.sql
    
The database file name can be configured in config.json. After initialising the database, 
import data from gzipped TSV files as follows:

    $ ./freebase_inject -z -c config.json -i FILE.tsv.gz

where config.json is the configuration file and FILE.tsv.gz is the input file. 
Option `-z` specifies whether the input file is gzip'ed or plain TSV. Import 
speed will typically be around 80k-100k statements/sec.    
    
## Start Server
    
The binary can be started both as a standalone application for testing and as FastCGI binary to use in
Apache or lighttpd. For testing purposes, simply start the server with the following command:
    
    $ cd build
    $ ./freebase_backend -c ../config.json    
    
You can now access the running system at http://localhost:8080 and issue REST requests, e.g.

    curl -X GET http://localhost:8080/entities/any

For deployment, please follow the instructions on the [CppCMS Webpage](http://cppcms.com/).    
