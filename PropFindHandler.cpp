#include <windows.h>

#undef min
#undef max

#include <string>

#include "ExpatImpl.h"
#include "PropFindHandler.hpp"

#if defined(_DEBUG)
#include <iostream>

using std::endl;
using std::string;
#endif

using namespace std::string_literals;

void PropFindHandler::OnPostCreate()
{
    EnableStartElementHandler();
    EnableEndElementHandler();
    EnableStartNamespaceDeclHandler();
    EnableEndNamespaceDeclHandler();
}

void PropFindHandler::OnStartElement(const XML_Char* name, XML_Char const** attributes)
{
    switch (xmlTreeDepth)
    {
    case 0:
	if (name != "propfind"s && name != "DAV:\037propfind"s)
	{
	    requestError = RequestError::InvalidRoot;
	    XML_StopParser(m_p, false);
	}

	break;

    case 1:
	if (name == "propname"s || name == "DAV:\037propname"s)
	    if (request != Request::None && request != Request::PropName)
	    {
		requestError = RequestError::ConflictingRequest;
		XML_StopParser(m_p, false);
	    }
	    else
		request = Request::PropName;
	else
	    if (name == "allprops"s || name == "DAV:\037allprops"s || name == "include"s || name == "DAV:\037include"s)
		if (request != Request::None && request != Request::AllProps)
		{
		    requestError = RequestError::ConflictingRequest;
		    XML_StopParser(m_p, false);
		}
		else
		{
		    request = Request::AllProps;
		    propListElement = (name == "include"s || name == "DAV:\037include"s);
		}
	    else
		if (name == "prop"s || name == "DAV:\037prop"s)
		    if (request != Request::None && request != Request::Prop)
		    {
			requestError = RequestError::ConflictingRequest;
			XML_StopParser(m_p, false);
		    }
		    else
		    {
			propListElement = true;
			request = Request::Prop;
		    }

	break;

    case 2:
	if (propListElement)
	{
	    XML_Char const* s = name;

	    while (*s && *s != asciiUnitSeparator[0])
		s++;

	    if (*s)
		properties[string(name, s)].insert(string(s + 1));
	    else
		properties["DAV:"s].insert(name);
	}

	break;

    default:
	break;
    }

    xmlTreeDepth++;

#if defined(_DEBUG)
    str.str(string());

    str << "Start element: " << name << endl;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str.str().data(), static_cast<DWORD>(str.str().length()), NULL, NULL);
#endif
}

void PropFindHandler::OnEndElement(const XML_Char *name)
{
    switch (xmlTreeDepth)
    {
    case 0:
	requestError = RequestError::UnmatchedClosingTag;
	XML_StopParser(m_p, false);

	break;

    case 2:
	if (propListElement)
	{
	    propListElement = false;

	    if (name != "include"s && name != "DAV:\037include"s && name != "prop"s && name != "DAV:\037prop"s)
	    {
		requestError = RequestError::UnmatchedClosingTag;
		XML_StopParser(m_p, false);
	    }
	}

	break;
    }

    xmlTreeDepth--;

#if defined(_DEBUG)
    str.str(string());

    str << "End element: " << name << endl;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str.str().data(), static_cast<DWORD>(str.str().length()), NULL, NULL);
#endif
}

void PropFindHandler::OnStartNamespaceDecl(XML_Char const *prefix, XML_Char const *URI)
{
    string &namespacePrefix = namespaces[URI];

    if (namespacePrefix.empty())
	namespacePrefix.assign(prefix ? prefix : "");

#if defined(_DEBUG)
    str.str(string());

    str << "Start namespace: " << (prefix ? prefix : "") << " => " << URI << endl;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str.str().data(), static_cast<DWORD>(str.str().length()), NULL, NULL);
#endif
}

void PropFindHandler::OnEndNamespaceDecl(XML_Char const *prefix)
{
#if defined(_DEBUG)
    str.str(string());

    str << "End namespace: " << (prefix ? prefix : "") << endl;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str.str().data(), static_cast<DWORD>(str.str().length()), NULL, NULL);
#endif
}

#if defined(_DEBUG)
PropFindHandler::PropFindHandler()
{
    static BOOL hasConsole = ::AllocConsole() && ::AttachConsole(::GetCurrentProcessId());
}

bool PropFindHandler::Parse(XML_Char const* xmlDocument, int xmlDocumentSize, bool isLastDocumentPart)
{
    xmlTreeDepth    = 0;
    propListElement = false;
    requestError    = RequestError::None;
    request	    = Request::None;

    namespaces.clear();
    properties.clear();

    bool parseResult = CExpatImpl<PropFindHandler>::Parse(xmlDocument, xmlDocumentSize, isLastDocumentPart);

    if (xmlTreeDepth && requestError == RequestError::None)
    {
	requestError = RequestError::UnmatchedOpenTag;
	parseResult = false;
    }

    return parseResult;
}

#endif