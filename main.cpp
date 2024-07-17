/**
 * Copyright (c) Institut national de l'information géographique et forestière https://www.ign.fr/
 *
 * File main authors:
 *  - JM Muller
 *  - C Meynard
 *
 * This file is part of Comp3D: https://github.com/IGNF/Comp3D
 *
 * Comp3D is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Comp3D is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Comp3D. If not, see <https://www.gnu.org/licenses/>.
 */

#include "compile.h"

#ifdef USE_QT
    #include <QTranslator>
    #include <QLocale>
    #include <QApplication>
    #include <QSettings>
#endif

#ifdef USE_GUI
    #include "gui/mainwindow.h"
#endif

#include <iostream>
#include <string>
#include <locale.h>
#include "src/project.h"

#ifdef _WIN32
#include <windows.h>
#endif


static int main_auto(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  #ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        (void)freopen("con","w",stdout);
        (void)freopen("con","w",stderr);
    }
  #endif
  #ifdef USE_QT
    // Get system default user language: will be used if never set by user
    QString userLang = QLocale().name().split("_")[0];  // we can always take the [0] element after a split
    if (userLang.length() < 2 || userLang.length() > 3)
        userLang = QStringList(SUPPORTED_LANG_CODE)[0];
    //force locale for QT
    QLocale::setDefault(QLocale::C);
  #endif
    //force locale for libc++
    std::locale::global(std::locale::classic());
    std::locale mylocale;
    std::cout.imbue(mylocale);//default streams were created before main, with wrong locale
    std::cerr.imbue(mylocale);
    std::cin.imbue(mylocale);
    //force locale for libc
    setlocale(LC_NUMERIC,"C");

    std::cout<<"\n------------------------------------------------------------------------"<<std::endl;
    std::cout<<" "<<COMP3D_VERSION<<" -- git commit "<<GIT_VERSION<<std::endl;
    std::cout<<"options: "<<COMP3D_OPTIONS<<" --  built "<<__DATE__<<" "<<__TIME__<<std::endl;
    std::cout<<COMP3D_COPYRIGHT<<"  --  "<<COMP3D_LICENSE<<std::endl;
    std::cout<<COMP3D_REPO<<std::endl;
    std::cout<<"------------------------------------------------------------------------\n"<<std::endl;

    int result=EXIT_SUCCESS;

  #ifdef USE_QT
    //try to read preferences file
    QCoreApplication::setOrganizationName(COMP3D_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(COMP3D_ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(COMP3D_APPLICATION_NAME);
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QSettings settings;
    userLang = settings.value("/Comp3D/Language",userLang).toString();
    settings.setValue("/Comp3D/Language",userLang); // save in case default value was used above
    Project::defaultLogLang = userLang.toStdString();

    QTranslator translator;
    translator.load(QString(":/i18n/Comp3D_%1.qm").arg(userLang));
    QTranslator translator2;
    translator2.load(QString(":/qt_%1.qm").arg(userLang));
    QTranslator translator3;
    translator3.load(QString(":/qtbase_%1.qm").arg(userLang));

    QCoreApplication *a;
    if (argc>1)
    {
      a=new QCoreApplication(argc, argv);
    }else{
      a=new QApplication(argc, argv); //QApplication needs x11!
    }
    a->installTranslator(&translator);
    a->installTranslator(&translator2);
    a->installTranslator(&translator3);
    setlocale(LC_NUMERIC,"C");//QApplication overwrites libc locale!!!
  #endif

#ifdef USE_GUI
    if (argc == 1) {
        MainWindow w((QApplication*)a);
        w.show();

        result=a->exec();
    } else
#endif
    {
#ifdef USE_AUTO
      result=main_auto(argc,argv);
#endif
    }
    std::cout<<"------------------------------------------------------------------------"<<std::endl;
    std::cout<<" end of "<<COMP3D_VERSION<<std::endl;
    std::cout<<"------------------------------------------------------------------------"<<std::endl;
#ifdef USE_QT
    delete a;
#endif
    return result;
}

static void usage(const std::string& msg = "")
{
    if (msg.size())
        std::cout << msg << std::endl;

#ifndef USE_QT
    std::cout<<"Options: [--proj=<dir>] [--lang=<code>] <project_path> " << std::endl;
    std::cout<<"    -p <dir> or --proj=<dir>  : installation directory of 'proj' if not standard" << std::endl;
#else
    std::cout<<"Options: [--lang=<code>] <project_path> " << std::endl;
#endif
    std::cout<<"    -l <code> or --lang=<code>: language for output report " << std::endl;
    std::cout<<"  Supported languages: ";
    for (auto p : SUPPORTED_LANG_CODE)
        std::cout << p << " ";
    std::cout << std::endl << std::endl;
}

static bool checkOption(const char *s, const char* l, char *argv[], int& i, int n, const char* &value)
{
    const char* arg=argv[i];

    auto len_s = strlen(s);
    auto len_l = strlen(l);
    auto len_a = strlen(arg);

    value=nullptr;
    if (strcmp(arg,s)==0 || strcmp(arg,l)==0) {
        if (i < n)
            value = argv[++i];
        return true;
    }
    if (strncmp(arg,s,len_s)==0) {  // then len_a > len_s as tested before in strcmp(arg,s)
        value = arg+len_s;
        return true;
    }

    if (strncmp(arg,l,len_l)==0 && len_a>len_l && arg[len_l] == '=') {
        if (len_a >len_l+1)
            value = arg+len_l+1;
        return true;
    }
    return false;
}

static int main_auto(int argc, char *argv[])
{
    const char *filename_arg = nullptr;
    bool force_lang=false;
    std::string pgm_name=argc ? argv[0] : COMP3D_APPLICATION_NAME;

    for (int i=1; i<argc; i++) {
        const char *arg=argv[i];
        const char *value;
        if (strcmp(arg,"-h") == 0 || strcmp(arg,"--help") == 0) {
            usage();
            std::cout<<"Informations: "<<std::endl;
            std::string json_data_str=Project::createTemplate("file.comp");
            std::cout<<"Minimal .comp file:\n--------------------"<<std::endl;
            std::cout<<"data=\n"<<json_data_str<<std::endl;
            std::cout<<"--------------------"<<std::endl;
            Projection::showAllProj();
            return 0;
#ifndef USE_QT
        } else if (checkOption("-p","--proj",argv,i,argc,value)) {
            if (value==nullptr || strlen(value) == 0) {
                usage(pgm_name + ": Missing parameter for option '" + arg);
                return 1;
            }
            Projection::setCmdLineProjPath(value);
#endif
        } else if (checkOption("-l","--lang",argv,i,argc,value)) {
            if (value==nullptr || strlen(value) == 0) {
                usage(pgm_name + ": Missing parameter for option '" + arg);
                return 1;
            }
            for (auto p : SUPPORTED_LANG_CODE) {
                if (strcmp(p,value)==0) {
                    Project::defaultLogLang = value;
                    force_lang = true;
                    break;
                }
            }
            if (! force_lang) {
                usage (pgm_name + ": Unsupported language code '" + value + "'");
                return 1;
            }
        } else if (strncmp(arg,"-",1) == 0) {
            usage(pgm_name + ": Unknown option '" + arg + "'");
            return 1;
        } else {
            if (filename_arg != nullptr) {
                usage(pgm_name + ": Extra argument '" + arg + "'");
            }
            filename_arg = arg;
        }
    }
    if (filename_arg == nullptr) {
        usage (pgm_name + ": Missing project file name");
        return 1;
    }

    std::string filename=filename_arg;

    std::cout<<"Try to compute "<<filename<<std::endl;
    std::ostringstream error_msg;

    Project project(filename);
    error_msg.str("");
    if (!project.read_config())
        return 1;
    bool readOk=false;
    error_msg.str("");
    readOk=(project.readData()
            && project.set_least_squares(project.config.internalConstr));
    if (!readOk)
    {
        std::cerr<<project.mInfo.toStrRaw();
        project.mInfo.clear();
        //return 1;
    }

    if (force_lang)
        project.config.lang = Project::defaultLogLang;

    if (!project.readyToCompute)
    {
        std::cerr<<"Errors during preparation, computation will not be done."<<std::endl;
        return 2;
    }
    project.computation(project.config.invert,true);
    if (!project.mInfo.isEmpty())
    {
        std::cerr<<project.mInfo.toStrRaw();
        return 1;
    }
  #ifdef USE_RES
    Project::prepareJson(project.filename,project.config.name);
  #endif
    std::cout<<"Computation duration: "<<to_simple_string(project.lsquares.computation_end-project.lsquares.computation_start)<<std::endl;
    return 0;
}
