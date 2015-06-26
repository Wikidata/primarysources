Primary Sources Tool for Wikidata
=================================

[![Build Status](https://travis-ci.org/google/primarysources.svg?branch=master)](https://travis-ci.org/google/primarysources)

[Freebase](https://freebase.com) was launched to be a “Wikipedia for structured data”, because in
2007 there was no such project. But now we do have Wikidata, and Wikidata
and its community is developing very fast. Today, the goals of Freebase
might be better served by supporting Wikidata.

See https://plus.google.com/109936836907132434202/posts/bu3z2wVqcQc

Freebase has seen a huge amount of effort go into it since it went public
in 2007. It makes a lot of sense to make the results of this work available
to Wikidata. But knowing Wikidata and its community a bit, it is obvious
that we can not and should not simply upload Freebase data to Wikidata:
Wikidata would prefer the data to be referenced to external, primary sources.

The Primary Sources Tool is an Open Source tool which will run on Wikimedia
labs and which will allow Wikidata contributors to find references for a
statement and then upload the statement and the reference to Wikidata. We
will release several sets of Freebase data ready for consumption by this tool
under a CC0 license. This tool should also work for statements already in
Wikidata without sufficient references, or for other datasets, like DBpedia
and other machine extraction efforts, etc.

Website and discussion of this tool:
https://www.wikidata.org/wiki/Wikidata:Primary_sources_tool
