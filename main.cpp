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
#include <getopt.h>
//#include <Eigen/Dense>
#include <locale.h>
#include "src/project.h"

#ifdef _WIN32
#include <windows.h>
#endif


int main_auto(int argc, char *argv[]);

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

void usage(const std::string& msg = "")
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

int main_auto(int argc, char *argv[])
{
    int c;
    bool force_lang=false;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
        {"help", no_argument,       0, 'h'},
        {"lang", required_argument, 0, 'l'},
#ifndef USE_QT
        {"proj", required_argument, 0, 'p'},
#endif
        {0,      0,                 0, 0 }};

        c = getopt_long(argc, argv,
#ifndef USE_QT
                        "hl:p:",
#else
                        "hl:",
#endif
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'h':
        {
            usage();
            std::cout<<"Informations: "<<std::endl;
            std::string json_data_str=Project::createTemplate("file.comp");
            std::cout<<"Minimal .comp file:\n--------------------"<<std::endl;
            std::cout<<"data=\n"<<json_data_str<<std::endl;
            std::cout<<"--------------------"<<std::endl;
            Projection::showAllProj();
            return 0;
        }
        case 'l':
            for (auto p : SUPPORTED_LANG_CODE) {
                if (strcmp(p,optarg)==0) {
                    Project::defaultLogLang = optarg;
                    force_lang = true;
                    break;
                }
            }
            if (! force_lang) {
                usage (std::string(argv[0]) + ": Unsupported language code '" + optarg + "'");
                return 1;
            }
            break;
        case 'p':
            Projection::setCmdLineProjPath(optarg);
            break;
        default:
            usage();
            return 1;
        }
    }

    if (optind != argc-1) {
        usage (std::string(argv[0]) + ": Missing project file name");
        return 1;
    }

    std::string filename=argv[optind];

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
