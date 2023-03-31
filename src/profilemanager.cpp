#include <string>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <list>

#include <iostream>

#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_utils.hpp>
#include <rapidxml/rapidxml_print.hpp>

#include "profilemanager.h"

#if DEBUG > 0
#  include <iostream>
#endif

const char ProfileManager::protocol[] { "v3 31 Oct 2022" };

ProfileManager::ProfileManager(std::string config)
    : profilesModified(false)
{
    // Determine source of profile data
    if (config.size() > 2 && config.substr(0,2).compare("~/") == 0)
        configPath =
        std::string(getenv("HOME")) + "/" + config.substr(2);
    else
        configPath = config;
    
#   if DEBUG > 0
        std::cout << "Using \"" << configPath <<"\" for profiles.\n";
#   endif
    
    // Check file exists or initialise it
    if (access(configPath.c_str(), F_OK) == -1) {
        std::ofstream of(configPath);
        of << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl
           << "<hsj>" << std::endl
           << "<protocol>" << protocol << "</protocol>" << std::endl
           << "</hsj>" << std::endl;
        of.close();
    }
    
    // Read the whole of the config file into a C string property
    configXML = new rapidxml::file<>(configPath.c_str());
    profilesDoc.parse<0>(configXML->data());
    
    //  Check the protocol
    rapidxml::xml_node<> *root { profilesDoc.first_node() };
    rapidxml::xml_node<> *node { root->first_node("protocol") };
    
#   if DEBUG > 0
    if (node)
        std::cout << "Configuration file Protocol: " << protocol << std::endl;
#   endif
    
    if (!node)
        throw std::runtime_error("No protocol tag in configuration file");
    
    if (strncmp(node->value(), protocol, strlen(protocol)) != 0)
        throw std::runtime_error("Protocol of configuration file is incompatible");
}

ProfileManager::~ProfileManager()
{
    writeProfiles();
    delete configXML;
}

rapidxml::xml_node<>* ProfileManager::exists(const std::string& name) const
{
    rapidxml::xml_node<> *root { profilesDoc.first_node() };
    rapidxml::xml_node<> *node;    
    
    for (node = root->first_node("profile");
         node && node->first_attribute("name")->value() != name;
         node = node->next_sibling("profile"))
         //std::cout << node->first_attribute("name")->value()
         ;
    
//     if (node == 0)
//         throw std::invalid_argument("No profile found");
        
    return node;
}

void ProfileManager::saveProfile(const std::string& name,
                                 const std::string& profile)
{
    //std::cout << "save profile " << name << std::endl;
    //std::cout << profile << std::endl;
    //std::cout << "\n\n\n";
    profilesModified = true;

    // root is the first (<hsj>) node of the existing profiles tree
    rapidxml::xml_node<>* root { profilesDoc.first_node() };
    
    // Remember the name of this profile
    char* n = { profilesDoc.allocate_string(name.c_str()) };
    
    // Parse the new profile, remembering its contents
    rapidxml::xml_document<> newProfile;
    char* p { newProfile.allocate_string(profile.c_str()) };
    newProfile.parse<0>(p);

    // If the profile exists already, delete it.
    rapidxml::xml_node<>* op { exists(name) };
    if (op)
        root->remove_node(op);
    
    // Get the first node of the parsed new profile and validate its name
    rapidxml::xml_node<>* sourceNode { newProfile.first_node() };
    if (strncmp(sourceNode->name(), "profile", 7) != 0) {
        throw std::runtime_error(
            "The XML root node of the profile to save wasn't <profile>"
        );
    }
    
    // Copy the node (but not the enclosed names and values (!))
    rapidxml::xml_node<>* content { profilesDoc.clone_node(sourceNode) };
    // Set its name attribute
    content->append_attribute(profilesDoc.allocate_attribute("name", n));
    // Sort the names and vaules out
    deep_copy_names_values(*sourceNode, *content, profilesDoc);
    
    // Add it to the xml tree
    root->append_node(content);

    writeProfiles();
}

std::string ProfileManager::getProfile(const std::string& name) const
{
    std::ostringstream os;
    rapidxml::xml_node<>* e = exists(name);
    // if profile does not exist, throw exception
    if (e == 0)
        throw std::invalid_argument("Profile '" + name + "' not found.");
    os << *e; 
    return os.str();
}

std::list<std::string> ProfileManager::profiles()
{
    std::list<std::string> names;

    // root is the first (<hsj>) node of the existing profiles tree
    rapidxml::xml_node<>* root { profilesDoc.first_node() };

    // first node of root is protocol, which we can ignore here
    rapidxml::xml_node<>* child { root->first_node() };

    // siblings of protocol are the profiles
    rapidxml::xml_node<>* sibling = child->next_sibling();

    // for (rapidxml::xml_node<>* sibling = child->next_sibling(); sibling != NULL; sibling->next_sibling()) {
    while (sibling != NULL) {
        rapidxml::xml_attribute<char>* att = sibling->first_attribute();
        std::string value = att->value();
        names.push_back(value);
        sibling = sibling->next_sibling();
    }
    return names;
}

void ProfileManager::writeProfiles()
{
    if (profilesModified) {
        std::ofstream of(configPath);
        of << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
        of << profilesDoc;
    }
    profilesModified = false;
}

//  Code from Coolfluid (http://coolfluid.github.io/)
//  to perform a deep clone of an XML node. This version
//  is modified to pass a reference to the memory allocating
//  document, which can be called upon to allocate memory
//  before the node is attached.
//  Source: 
//   http://coolfluid.github.io/doxygen/utest-rapidxml_8cpp_source.html
void ProfileManager::deep_copy_names_values(rapidxml::xml_node<>& in,
                                            rapidxml::xml_node<>& out, 
                                            rapidxml::xml_document<>& mem)
{

    char* nname  { mem.allocate_string(in.name()) };
    char* nvalue { mem.allocate_string(in.value()) };
    out.name(nname);
    out.value(nvalue);

    // copy names and values of the attributes
    rapidxml::xml_attribute<>* iattr { in.first_attribute() };
    rapidxml::xml_attribute<>* oattr { out.first_attribute() };

    for ( ; iattr ; iattr = iattr->next_attribute(),
                    oattr = oattr->next_attribute() )
    {
        char* aname  { mem.allocate_string(iattr->name()) };
        char* avalue { mem.allocate_string(iattr->value()) };

        oattr->name(aname);
        oattr->value(avalue);
    }

    // copy names and values of the child nodes
    rapidxml::xml_node<> * inode { in.first_node() };
    rapidxml::xml_node<> * onode { out.first_node() };

    for ( ; inode ; inode = inode->next_sibling(),
                    onode = onode->next_sibling() )
    {
        deep_copy_names_values(*inode, *onode, mem);
    }
}
