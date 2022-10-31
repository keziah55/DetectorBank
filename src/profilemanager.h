/*!
 * profilemanager.h
 */
#ifndef _PROFILEMANAGER_H_
#define _PROFILEMANAGER_H_

#include <string>
#include <list>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"

/*!
 * The ProfileManager class is responsible for reading and writing
 * the XML-serialised description of a DetectorBank
 */
class ProfileManager {
public:
    /*!
     * Create a profile manager to save and restore properties
     * of DetectorBanks
     * \param config Path XML file where configurations are stored.
     */
    ProfileManager(std::string config
                    = std::string("~/.config/hopfskipjump.xml"));
    /*! Destroy this profile manager, rewriting the configuration
     *  file if any new profiles have been added
     */
    ~ProfileManager();
    /*! Protocol version in use by this release.
     * 
     * The protocol version must agree with any file which the
     * DetectorBank attemts to load.
     */
    static const char protocol[];
    /*!
     * Determine whether the name profile exists in the database
     * \param name Name of the profile to find
     * \returns The address of xml profile description or null if absent.
     */
    rapidxml::xml_node<>* exists(const std::string& name) const;
    
    /*!
     * Save a serialised profile under a given name, possibly overwriting
     * an existing profile.
     * \param name Name of the profile to create
     * \param profile Text describing the new profile
     * \throw std::string The XML root node of the profile to save
     *                    wasn't \<profile\>
     */
    void saveProfile(const std::string& name, const std::string& profile);
    /*!
     * Get an XML representation of a profile which can be used
     * by fromXML()
     * \param name The profile's name
     * \returns A string of XML describing the profile
     * \throw std::string Profile 'name' not found.
     */
    std::string getProfile(const std::string& name) const;

    /*!
     * Return list of existing profiles
     * \return List of strings
     */
    std::list<std::string> profiles();
    
protected:
    /*!
     * Recursively clone a node and its values
     * \param in Source node
     * \param out Target node
     * \param mem Document supplying the memory
     */
    void deep_copy_names_values(rapidxml::xml_node<>& in,
                                rapidxml::xml_node<>& out,
                                rapidxml::xml_document<>& mem);
    std::string configPath;               /*!< Path of the configuration file */
    rapidxml::file<>* configXML;          /*!< Known configurations */
    rapidxml::xml_document<> profilesDoc; /*!< XML doc of known configurations */
    bool profilesModified;                /*!< Document tree has been modified */
};

#endif
