#if !defined(SCROLL_WINDOW_MSG_PROP_FIND_HANDLER_HPP)
#define SCROLL_WINDOW_MSG_PROP_FIND_HANDLER_HPP

#include <string>
#include <map>
#include <set>

#if defined(_DEBUG)
# include <sstream>
#endif

#include "ExpatImpl.h"

class PropFindHandler: public CExpatImpl<PropFindHandler>
{
    friend class CExpatImpl<PropFindHandler>;

public:
    enum class Request { None, Prop, PropName, AllProps };
    enum class RequestError { None, InvalidRoot, ConflictingRequest, UnmatchedOpenTag, UnmatchedClosingTag };

protected:
    unsigned xmlTreeDepth = 0;
    bool propListElement = false;
    RequestError requestError = RequestError::None;

    std::map<std::string, std::string> namespaces;
    Request request = Request::None;
    std::map<std::string, std::set<std::string>> properties;

    void OnPostCreate();
    void OnStartElement(XML_Char const *name, XML_Char const **attributes);
    void OnEndElement(XML_Char const *name);
    void OnStartNamespaceDecl(XML_Char const *prefix, XML_Char const *URI);
    void OnEndNamespaceDecl(XML_Char const *prefix);

#if defined(_DEBUG)
    std::ostringstream str;

public:
    PropFindHandler();
#endif

public:
    Request requestType() const;
    RequestError error() const;
    std::string const &namespacePrefix(std::string const& ns) const;
    std::map<std::string, std::set<std::string>> const &propertiesMap() const;

    bool Create(XML_Char const* defaultDocumentEncoding);
    bool Parse(XML_Char const* xmlDocument, int xmlDocumentSize = -1, bool isLastDocumentPart = true);
};

inline PropFindHandler::Request PropFindHandler::requestType() const
{
    return request;
}

inline PropFindHandler::RequestError PropFindHandler::error() const
{
    return requestError;
}
inline std::string const &PropFindHandler::namespacePrefix(std::string const &ns) const
{
    static const std::string emptyString;

    auto it = namespaces.find(ns);

    if (it == namespaces.end())
	return emptyString;

    return it->second;
}

inline std::map<std::string, std::set<std::string>> const& PropFindHandler::propertiesMap() const
{
    return properties;
}

inline XML_Char const asciiUnitSeparator[] = "\037";

inline bool PropFindHandler::Create(XML_Char const* defaultDocumentEncoding)
{
    return CExpatImpl<PropFindHandler>::Create(defaultDocumentEncoding, asciiUnitSeparator);
}

#endif // !defined(SCROLL_WINDOW_MSG_PROP_FIND_HANDLER_HPP)