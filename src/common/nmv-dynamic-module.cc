/* -*- Mode: C++; indent-tabs-mode:nil; c-basic-offset:4;-*- */

/*Copyright (c) 2005-2006 Dodji Seketeli
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission
 * notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <string>
#include <map>
#include <glibmm.h>
#include "nmv-exception.h"
#include "nmv-dynamic-module.h"
#include "nmv-libxml-utils.h"
#include "nmv-env.h"

using namespace std;

namespace nemiver {
namespace common {

struct ModuleRegistry::Priv {
    map<std::string, DynamicModule::ConfigSafePtr> config_map ;
    Glib::Mutex cache_mutex;
    map<UString, GModule*> library_cache ;
};//end ModuleRegistry::Priv

ModuleRegistry::ModuleRegistry () :
    m_priv (new ModuleRegistry::Priv)
{
}

ModuleRegistry::~ModuleRegistry ()
{
}

GModule*
ModuleRegistry::get_library_from_cache (const UString a_name)
{
    GModule *module (0);
    std::map<UString, GModule*>::iterator it =
                                    m_priv->library_cache.find (a_name);
    if (it != m_priv->library_cache.end ()) {
        module = it->second;
    }
    return module;
}

void
ModuleRegistry::put_library_into_cache (const UString a_name,
                                        GModule *a_library)
{
    THROW_IF_FAIL (a_name != "");
    Glib::Mutex::Lock lock (m_priv->cache_mutex);
    m_priv->library_cache[a_name] = a_library;
}

struct DynamicModule::Loader::Priv {
    vector<UString> config_search_paths;
    map<std::string, DynamicModule::ConfigSafePtr> config_map ;
    vector<UString> module_library_path;
    DynamicModuleManager * module_manager;

    Priv () :
        module_manager (0)
    {}
};//end struct Loader::Priv

DynamicModule::Loader::Loader () :
    m_priv (new Loader::Priv)
{

    //init the module config search path to system config dir
    config_search_paths ().push_back (env::get_system_config_dir ());
}

DynamicModule::Loader::~Loader ()
{
}

void
DynamicModule::Loader::set_dynamic_module_manager (DynamicModuleManager *a_mgr)
{
    THROW_IF_FAIL (m_priv);
    m_priv->module_manager = a_mgr;
}

DynamicModuleManager*
DynamicModule::Loader::get_dynamic_module_manager ()
{
    THROW_IF_FAIL (m_priv);
    return m_priv->module_manager;
}

DynamicModule::ConfigSafePtr
DynamicModule::Loader::module_config (const string &a_module_name)
{
    ConfigSafePtr result;
    if (a_module_name == "") {return result;}

    map<string, ConfigSafePtr>::iterator iter =
                                    m_priv->config_map.find (a_module_name);
    if (iter == m_priv->config_map.end ()) {
        //we didn't find the module config in config cache.
        //Let's walk the module conf paths
        //and try to parse module confs from there.
        for (vector<UString>::const_iterator it = config_search_paths ().begin ();
             it != config_search_paths ().end ();
             ++it) {
            vector<string> path_elements;
            path_elements.push_back (it->c_str ());
            path_elements.push_back (a_module_name + ".conf");
            string path = Glib::build_filename (path_elements);
            if (!Glib::file_test (path, Glib::FILE_TEST_EXISTS)) {continue;}

            result = parse_module_config_file (path.c_str ());
            if (!result) {return result;}

            m_priv->config_map[a_module_name] = result;
            break;
        }
    } else {
        result = iter->second;
    }
    return result;
}

vector<UString>&
DynamicModule::Loader::config_search_paths ()
{
    return m_priv->config_search_paths;
}

UString
DynamicModule::Loader::build_library_path (const UString &a_module_name,
                                           const UString &a_lib_name)
{
    DynamicModule::ConfigSafePtr mod_conf = module_config (a_module_name);
    THROW_IF_FAIL (mod_conf);

    UString path;
    vector<UString>::iterator it, end;
    vector<UString> search_path;
    if (mod_conf->custom_library_search_paths.size ()) {
        it = mod_conf->custom_library_search_paths.begin ();
        end = mod_conf->custom_library_search_paths.end ();
    } else {
        it = config_search_paths ().begin ();
        end = config_search_paths ().end ();
    }

    for (; it != end; ++it) {
        LOG_D ("in directory '" << Glib::locale_from_utf8 (*it) << "' ...",
               "module-loading-domain");
        common::GCharSafePtr system_lib_path
            (g_module_build_path (it->c_str (), a_lib_name.c_str ()));
        LOG_D ("looking for library '" << Glib::locale_from_utf8 (system_lib_path.get ()),
               "module-loading-domain");
        if (Glib::file_test (Glib::filename_from_utf8 (system_lib_path.get ()),
                             Glib::FILE_TEST_EXISTS)) {
            return system_lib_path.get ();
        }
    }
    LOG ("Could not find library " + a_lib_name);
    return "";
}

DynamicModule::ConfigSafePtr
DynamicModule::Loader::parse_module_config_file (const UString &a_path)
{
    if (a_path == "") {
        THROW ("Got path \"\" to modules config file");
    }

    using libxmlutils::XMLTextReaderSafePtr;
    using libxmlutils::XMLCharSafePtr;
    XMLTextReaderSafePtr reader;
    UString name;
    libxmlutils::XMLCharSafePtr xml_str;

    reader.reset (xmlNewTextReaderFilename (a_path.c_str ()));
    if (!reader) {
        THROW ("could not create xml reader");
    }

    if (!goto_next_element_node_and_check (reader, "moduleconfig")) {
        THROW ("modules config file "
               + a_path + " root element must be 'moduleconfig'");
    }

    UString module_name, library_name;
    DynamicModule::ConfigSafePtr config (new Config);

    if (!goto_next_element_node_and_check (reader, "module")) {
        THROW ("expected 'module' node as child of 'moduleconfig' node");
    }
    if (!goto_next_element_node_and_check (reader, "name")) {
        THROW ("expected 'name' node as child of 'module' node");
    }
    xml_str.reset (xmlTextReaderReadString (reader.get ()));
    module_name = reinterpret_cast<const char*> (xml_str.get ());

    if (!goto_next_element_node_and_check (reader, "libraryname")) {
        THROW ("expected 'libraryname' node as second child "
               "of 'module' node");
    }
    xml_str.reset (xmlTextReaderReadString (reader.get ()));
    library_name = reinterpret_cast<const char*> (xml_str.get ());
    if (module_name != "" && library_name != "") {
        config->library_name = library_name;
    }
    module_name = library_name = "";

    if (!goto_next_element_node_and_check (reader, "customsearchpaths")) {
        THROW ("expected 'customsearchpaths' after moduleconfig"
               "of 'module' node");
    }
    UString path;
    name = reinterpret_cast<const char*> (xmlTextReaderConstName (reader.get ()));
    if (name == "customsearchpaths") {
        for (;;) {
            if (goto_next_element_node_and_check (reader, "path")) {
                xml_str.reset (xmlTextReaderReadString (reader.get ()));
                path = reinterpret_cast<const char*> (xml_str.get ());
                if (path != "") {
                    config->custom_library_search_paths.push_back (path);
                }
            } else {
                break;
            }
        }
    }
    return config;
}

UString
DynamicModule::Loader::module_library_path (const UString &a_module_name)
{
    UString library_name, library_path;
    DynamicModule::ConfigSafePtr mod_conf = module_config (a_module_name);
    THROW_IF_FAIL2 (mod_conf,
                    UString ("couldn't get module config for module ")
                    + a_module_name);

    //***********************************************
    //get the library name from the module names map
    //***********************************************
    library_name = mod_conf->library_name;
    library_path = build_library_path (a_module_name, library_name);
    return library_path;
}

GModule*
DynamicModule::Loader::load_library_from_path (const UString &a_library_path)
{
    if (!g_module_supported ()) {
        THROW ("We don't support dynamic modules on this platform");
    }
    GModule *module = g_module_open (a_library_path.c_str (),
            static_cast<GModuleFlags> (G_MODULE_BIND_LAZY));
    if (!module) {
        THROW (UString ("failed to load shared library ") + a_library_path
               + ": " + Glib::locale_from_utf8 (g_module_error ()));
    }
    g_module_make_resident (module);//we don't want to unload a module for now
    LOG_D ("loaded module at path: " << Glib::locale_from_utf8 (a_library_path),
           "module-loading-domain");
    return module;
}

GModule*
DynamicModule::Loader::load_library_from_module_name (const UString &a_name)
{
    UString library_path = module_library_path (a_name);
    if (library_path == "") {
        THROW ("Couldn't find library for module " + a_name);
    }
    GModule *lib = load_library_from_path (library_path);
    if (!lib) {
        THROW (UString ("failed to load shared library ") + library_path);
    }
    LOG_D ("loaded module " << Glib::locale_from_utf8 (a_name), "module-loading-domain");
    return lib;
}

DynamicModuleSafePtr
DynamicModule::Loader::create_dynamic_module_instance (GModule *a_module)
{
    THROW_IF_FAIL (a_module);

    //***********************************************
    //get a pointer on the factory function exposes by
    //the module.
    //***********************************************
    gpointer factory_function = 0;
    if (!g_module_symbol (a_module,
                "nemiver_common_create_dynamic_module_instance",
                &factory_function) || !factory_function) {
        THROW (UString ("The library ")
                + g_module_name (a_module)
                + " doesn't export the symbol "
                "nemiver_common_create_dynamic_module_instance");
    }

    //**************************************************
    //call the factory function to create an intance of
    //nemiver::common::DynamicModule
    //**************************************************
    DynamicModule *loadable_module (0);
    ((bool (*) (void**)) factory_function) ((void**) &loadable_module);

    if (!loadable_module) {
        THROW (UString ("The instance factory of module ")
                + g_module_name (a_module)
                + " returned a NULL instance pointer of LoadableModle");
    }
    DynamicModuleSafePtr safe_ptr (loadable_module);
    if (!dynamic_cast<DynamicModule*> (loadable_module)) {
        //this causes a memory leak because we can't destroy loadable module
        //as we don't know its actual type.
        THROW (UString ("The instance factory of module ")
                + g_module_name (a_module)
                + " didn't return an instance of DynamicModule");
    }

    //**********************************************
    //return a safe pointer DynamicModuleSafePtr
    //**********************************************
    LOG_REF_COUNT (safe_ptr, g_module_name (a_module));
    return safe_ptr;
}

DynamicModuleSafePtr
DynamicModule::Loader::load (const UString &a_name)
{
    GModule *lib (0);
    lib = load_library_from_module_name (a_name);
    if (!lib) {
        LOG ("could not load the dynamic library of the dynmod '" +a_name+ "'");
        return DynamicModuleSafePtr (0);
    }

    DynamicModuleSafePtr result (create_dynamic_module_instance (lib));
    if (result) {
        result->set_module_loader (this);
    }
    return result;
}

DynamicModuleSafePtr
DynamicModule::Loader::load_from_path (const UString &a_lib_path)
{
    GModule *lib (0);
    lib = load_library_from_path (a_lib_path);
    if (!lib) {
        LOG ("could not load the dynamic library of the dynmod '"
             +a_lib_path+ "'");
        return DynamicModuleSafePtr (0);
    }
    LOG_D ("loaded module from path: "<< Glib::locale_from_utf8 (a_lib_path), "module-loading-domain");
    return create_dynamic_module_instance (lib);
}


class DefaultModuleLoader : public DynamicModule::Loader {
    //non copyable;
    DefaultModuleLoader (const DefaultModuleLoader &);
    DefaultModuleLoader& operator= (const DefaultModuleLoader &);

public:
    DefaultModuleLoader () {};
    virtual ~DefaultModuleLoader () {};
};//end DefaultLoader

struct DynamicModule::Priv {
    UString real_library_path;
    UString name;
    DynamicModule::Loader *loader;

    Priv () :
        loader (0)
    {}
};//end struct DynamicModule::Priv

DynamicModule::DynamicModule () :
    m_priv (new DynamicModule::Priv)
{
}

DynamicModule::~DynamicModule ()
{
    LOG_D ("deleted", "destructor-domain");
}
const UString&
DynamicModule::get_real_library_path () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->real_library_path;
}

void
DynamicModule::set_real_library_path (const UString &a_path)
{
    THROW_IF_FAIL (m_priv);
    m_priv->real_library_path = a_path;
}

void
DynamicModule::set_name (const UString &a_name)
{
    THROW_IF_FAIL (m_priv);
    m_priv->name = a_name;
}

const UString&
DynamicModule::get_name () const
{
    THROW_IF_FAIL (m_priv);
    return m_priv->name;
}

void
DynamicModule::set_module_loader (DynamicModule::Loader *a_loader)
{
    THROW_IF_FAIL (m_priv);
    m_priv->loader = a_loader;
}

DynamicModule::Loader*
DynamicModule::get_module_loader ()
{
    THROW_IF_FAIL (m_priv);
    return m_priv->loader;
}

struct DynamicModuleManager::Priv {
    ModuleRegistry registry;
    DynamicModule::LoaderSafePtr loader;

    DynamicModule::LoaderSafePtr
    get_module_loader ()
    {
        if (!loader) {
            loader.reset (new DefaultModuleLoader);
        }
        THROW_IF_FAIL (loader);
        return loader;
    }
};//end struct DynamicModuleManager::Priv

DynamicModuleManager::DynamicModuleManager () :
    m_priv (new DynamicModuleManager::Priv)
{
}

DynamicModuleManager::~DynamicModuleManager ()
{
}

DynamicModuleSafePtr
DynamicModuleManager::load_module (const UString &a_name,
                                   DynamicModule::Loader &a_loader)
{
    GModule *lib = module_registry ().get_library_from_cache (a_name);
    if (!lib) {
        //dll doesn't exist in library cache.
        //try to load the dll from disk and put it in library cache.
        lib = a_loader.load_library_from_module_name (a_name);
        if (lib) {
            module_registry ().put_library_into_cache (a_name, lib);
        }
    }
    if (!lib) {
        LOG_ERROR
            ("could not load the dynamic library of the dynmod '" +a_name+ "'");
        return DynamicModuleSafePtr (0);
    }
    DynamicModuleSafePtr module = a_loader.create_dynamic_module_instance (lib);
    THROW_IF_FAIL (module);
    LOG_REF_COUNT (module, a_name);

    module->set_module_loader (&a_loader);
    module->set_name (a_name);
    module->set_real_library_path (a_loader.module_library_path (a_name));
    a_loader.set_dynamic_module_manager (this);
    LOG_REF_COUNT (module, a_name);

    LOG_D ("loaded module " << Glib::locale_from_utf8 (a_name),
            "module-loading-domain");
    return module;
}

DynamicModuleSafePtr
DynamicModuleManager::load_module (const UString &a_name)
{
    LOG_D ("loading module " << Glib::locale_from_utf8 (a_name),
           "module-loading-domain");
    return load_module (a_name, *module_loader ());
}

DynamicModuleManager&
DynamicModuleManager::get_default_manager ()
{
    static DynamicModuleManager s_default_dynmod_mgr;
    return s_default_dynmod_mgr;
}

DynamicModuleSafePtr
DynamicModuleManager::load_module_with_default_manager
                                            (const UString &a_mod_name,
                                             DynamicModule::Loader &a_loader)
{
    return get_default_manager ().load_module (a_mod_name, a_loader);
}

DynamicModuleSafePtr
DynamicModuleManager::load_module_with_default_manager (const UString &a_mod_name)
{
    return get_default_manager ().load_module (a_mod_name);
}

DynamicModuleSafePtr
DynamicModuleManager::load_module_from_path (const UString &a_library_path,
                                             DynamicModule::Loader &a_loader)
{
    GModule *lib = a_loader.load_library_from_path (a_library_path);
    if (!lib) {
        LOG ("could not load dynamic library '" + a_library_path + "'");
        return DynamicModuleSafePtr (0);
    }
    a_loader.set_dynamic_module_manager (this);
    DynamicModuleSafePtr module = a_loader.create_dynamic_module_instance (lib);
    module->set_module_loader (&a_loader);
    LOG_D ("loaded module from path " << Glib::locale_from_utf8 (a_library_path),
           "module-loading-domain");

    return module;
}

DynamicModuleSafePtr
DynamicModuleManager::load_module_from_path (const UString &a_library_path)
{
    LOG_D ("loading module from path " << Glib::locale_from_utf8 (a_library_path),
           "module-loading-domain");
    return load_module_from_path (a_library_path, *module_loader ());
}


ModuleRegistry&
DynamicModuleManager::module_registry ()
{
    return m_priv->registry;
}

DynamicModule::LoaderSafePtr&
DynamicModuleManager::module_loader ()
{
    if (!m_priv->loader) {
        m_priv->loader.reset (new DefaultModuleLoader);
    }
    THROW_IF_FAIL (m_priv->loader);
    return m_priv->loader;
}

void
DynamicModuleManager::module_loader (DynamicModule::LoaderSafePtr &a_loader)
{
    m_priv->loader = a_loader;
}

}//end namespace common
}//end namespace nemiver

