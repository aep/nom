#include <QCoreApplication>
#include <QTextStream>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>

#include "make/lexer.hpp"
#include "make/parser.hpp"

#include "make/semantics.hpp"


QList<QByteArray> my_dir(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (that->lexer->source.isEmpty()) {
        throw ParserException("my-dir called post-parse");
    }

    return QList<QByteArray>() << QFileInfo(that->lexer->source.top().fileName).absolutePath().toLocal8Bit();
}
QList<QByteArray> all_these_files_under (MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 2) {
        throw ParserException("expecting exactly 2 arguments to function all-these-under");
    }


    QList<QByteArray> ret;
    QStringList filter;
    foreach(QByteArray a, args.at(1)) {
        filter.append(QString::fromUtf8(a));
    }


    foreach (const QByteArray &path, args.at(0)) {
        QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            it.next();
            QDirIterator it2(it.filePath(), filter, QDir::Files);
            while (it2.hasNext()) {
                it2.next();
                ret.append(it2.filePath().toLocal8Bit());
            }
        }
    }
    return ret;

}

QList<QByteArray> all_makefiles_under (MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 1) {
        throw ParserException("expecting exactly one argument to function all-makefiles-under");
    }

    if (args.at(0).count()  != 1) {
        throw ParserException("expecting exactly one argument to function all-makefiles-under");
    }

    args.append(QList<QByteArray>() <<  "Android.mk");
    return all_these_files_under(that, args);
}

QList<QByteArray> all_java_files_under(MakefileParser *that, QList<QList<QByteArray> > args)
{
    args.append(QList<QByteArray>() <<  "*.java");
    return all_these_files_under(that, args);
}

QList<QByteArray> all_html_files_under(MakefileParser *that, QList<QList<QByteArray> > args)
{
    args.append(QList<QByteArray>() <<  "*.html");
    return all_these_files_under(that, args);
}

QList<QByteArray> all_proto_files_under(MakefileParser *that, QList<QList<QByteArray> > args)
{
    args.append(QList<QByteArray>() <<  "*.proto");
    return all_these_files_under(that, args);
}
QList<QByteArray> all_renderscript_files_under(MakefileParser *that, QList<QList<QByteArray> > args)
{
    args.append(QList<QByteArray>() <<  "*.rs");
    return all_these_files_under(that, args);
}

QList<QByteArray> all_subdir_java_files(MakefileParser *that, QList<QList<QByteArray> > args)
{
    args.append(my_dir(that, args));
    return all_java_files_under(that, args);
}

QList<QByteArray> all_subdir_makefiles (MakefileParser *that, QList<QList<QByteArray> > args)
{
    args.append(my_dir(that, args));
    return all_makefiles_under(that, args);
}

QList<QByteArray> all_subdir_html_files(MakefileParser *that, QList<QList<QByteArray> > args)
{
    args.append(my_dir(that, args));
    return all_html_files_under(that, args);
}


QList<QByteArray> first_makefiles_under(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 1)
        throw ParserException("expecting exactly one argument to function first-makefiles-under");

    //FIXME
    return QList<QByteArray>();
}

QList<QByteArray> intermediates_dir_for(MakefileParser *that, QList<QList<QByteArray> > args)
{
    return QList<QByteArray>() << "intermediates_dir_for";
}

QList<QByteArray> local_intermediates_dir(MakefileParser *that, QList<QList<QByteArray> > args)
{
    return QList<QByteArray>() << "local_intermediates_dir";
}

QList<QByteArray> include_path_for(MakefileParser *that, QList<QList<QByteArray> > args)
{
    return QList<QByteArray>() << "include_path_for";
}


//builtins
QList<QByteArray> strip(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () !=  1)
        throw ParserException("expecting exactly one argument list to function strip");

    QList<QByteArray> r;
    foreach (QByteArray a, args.at(0)) {
        if (!a.simplified().isEmpty())
            r.append(a);
    }
    return r;
}

QList<QByteArray> shell(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () <  1)
        throw ParserException("expecting arguments to function shell");

    qWarning("shell not implemented");

    //FIXME
    return QList<QByteArray>();
}

QList<QByteArray> filter_out(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 2)
        throw ParserException("expecting 2 arguments to function filter_out");


    QList<QByteArray> result;
    QList<QByteArray> excl = args.at(0);
    QList<QByteArray> in = args.at(1);


    foreach (QByteArray a, in) {
        //FIXME: apply gnumake filter expression
        if (!excl.contains(a))
            result.append(a);
    }
    return result;
}

QList<QByteArray> filter(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 2)
        throw ParserException("expecting 2 arguments to function filter");


    QList<QByteArray> result;
    QList<QByteArray> excl = args.at(0);
    QList<QByteArray> in = args.at(1);


    foreach (QByteArray a, in) {
        //FIXME: apply gnumake filter expression
        if (excl.contains(a))
            result.append(a);
    }
    return result;
}

QList<QByteArray> addsuffix (MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 2)
        throw ParserException("expecting 2 arguments to function addsuffix");

    if (args.at(0).count () != 1)
        throw ParserException("arg0 to addsuffix cannot contain spaces");

    QList<QByteArray> result;
    QByteArray suffix = args.at(0).at(0);
    QList<QByteArray> in = args.at(1);


    foreach (QByteArray a, in) {
        result.append(a + suffix) ;
    }
    return result;
}

QList<QByteArray> addprefix (MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 2)
        throw ParserException("expecting 2 arguments to function addprefix");

    if (args.at(0).count () != 1)
        throw ParserException("arg0 to addprefix cannot contain spaces");

    QList<QByteArray> result;
    QByteArray prefix = args.at(0).at(0);
    QList<QByteArray> in = args.at(1);


    foreach (QByteArray a, in) {
        result.append(prefix + a) ;
    }
    return result;
}

QList<QByteArray> wildcard (MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () != 1)
        throw ParserException("expecting 1 argument to function wildcard");

    QList<QByteArray> result;
    QList<QByteArray> patterns = args.at(0);

    //FIXME
    return result;
}

QList<QByteArray> patsubst(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () !=  3)
        throw ParserException("expecting exactly 3 argument lists to function patsubst");

    if (args.at(0).count() != 1)
        throw ParserException("argumentlist to arg0 of function pathsubst must be length 1");

    if (args.at(1).count() > 1)
        throw ParserException("argumentlist to arg1 of function pathsubst must be max length 1");

    QList<QByteArray> result;
    QByteArray pattern = args.at(0).at(0);
    QByteArray replacement;
    if (args.at(1).count()) replacement =  args.at(1).at(0);
    QList<QByteArray> in = args.at(2);

    foreach (QByteArray a, in) {
        //FIXME
        qWarning("pathsubst not implemented");
        result.append(a);
    }
    return result;
}

QList<QByteArray> dir(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () !=  1)
        throw ParserException("expecting exactly 1 argument list to function dir");

    QList<QByteArray> result;
    QList<QByteArray> in = args.at(0);

    foreach (const QByteArray &a, in) {
        result.append(QFileInfo(a).dir().path().toUtf8());
    }
    return result;
}

QList<QByteArray> notdir(MakefileParser *that, QList<QList<QByteArray> > args)
{
    if (args.count () !=  1)
        throw ParserException("expecting exactly 1 argument list to function dir");

    QList<QByteArray> result;
    QList<QByteArray> in = args.at(0);

    foreach (const QByteArray &a, in) {
        result.append(QFileInfo(a).completeBaseName().toUtf8());
    }
    return result;
}

QList<QByteArray> dummy(MakefileParser *that, QList<QList<QByteArray> > args)
{
    return QList<QByteArray>();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (app.arguments().count() < 2) {
        QTextStream(stderr) << "usage: blurp Android.mk" << endl;
        return 3;
    }

    QFile *in = new QFile(app.arguments().at(1));
    if (!in->open(QFile::ReadOnly)) {
        qFatal("cant open %s", qPrintable(in->fileName()));
    }

    /*
    QFile out(app.arguments().at(2));
    if (!out.open(QFile::WriteOnly | QFile::Truncate)) {
        qFatal("cant open %s", qPrintable(out.fileName()));
    }
*/

    MakefileLexer  lex(in, app.arguments().at(1).toLocal8Bit());
    MakefileParser ast;
    ast.context.functions.insert("all-makefiles-under", &all_makefiles_under);
    ast.context.functions.insert("all-java-files-under", &all_java_files_under);
    ast.context.functions.insert("find-other-java-files", &all_java_files_under);
    ast.context.functions.insert("find-other-html-files", &all_html_files_under);
    ast.context.functions.insert("all-proto-files-under", &all_proto_files_under);
    ast.context.functions.insert("all-renderscript-files-under", &all_renderscript_files_under);
    ast.context.functions.insert("all-subdir-java-files", &all_subdir_java_files);
    ast.context.functions.insert("all-subdir-makefiles", &all_subdir_makefiles);
    ast.context.functions.insert("all-subdir-html-files", &all_subdir_html_files);
    ast.context.functions.insert("first-makefiles-under", &first_makefiles_under);
    ast.context.functions.insert("intermediates-dir-for", &intermediates_dir_for);
    ast.context.functions.insert("local-intermediates-dir", &local_intermediates_dir);
    ast.context.functions.insert("include-path-for", &include_path_for);
    ast.context.functions.insert("java-lib-deps", &include_path_for);
    ast.context.functions.insert("private-function-all-cpp-files-under", &dummy);
    ast.context.functions.insert("all-harmony-test-java-files-under", &dummy);
    ast.context.functions.insert("all-src-files", &dummy);
    ast.context.functions.insert("libcore_to_document", &dummy);
    ast.context.functions.insert("junit_to_document", &dummy);
    ast.context.functions.insert("find-subdir-assets", &dummy);
    ast.context.functions.insert("cts-get-lib-paths", &dummy);
    ast.context.functions.insert("all-named-subdir-makefiles", &dummy);
    ast.context.functions.insert("all-main-java-files-under", &dummy);
    ast.context.functions.insert("all-core-resource-dirs", &dummy);
    ast.context.functions.insert("all-test-java-files-under", &dummy);
    ast.context.functions.insert("harmony-test-resource-dirs", &dummy);
    ast.context.functions.insert("cts-get-package-paths", &dummy);
    ast.context.functions.insert("cts-get-native-paths", &dummy);
    ast.context.functions.insert("cts-get-test-xmls", &dummy);
    ast.context.functions.insert("all-files-under", &dummy);
    ast.context.functions.insert("my-dir", &my_dir);
    ast.context.functions.insert("strip", &strip);
    ast.context.functions.insert("shell", &shell);
    ast.context.functions.insert("filter-out", &filter_out);
    ast.context.functions.insert("filter", &filter);
    ast.context.functions.insert("wildcard", &wildcard);
    ast.context.functions.insert("patsubst", &patsubst);
    ast.context.functions.insert("addsuffix", &addsuffix);
    ast.context.functions.insert("addprefix", &addprefix);
    ast.context.functions.insert("dir", &dir);
    ast.context.functions.insert("notdir", &notdir);
    ast.context.functions.insert("foreach", &dummy);
    ast.context.functions.insert("subst", &dummy);
    ast.context.functions.insert("sort", &dummy);
    ast.context.functions.insert("word", &dummy);
    ast.context.functions.insert("if", &dummy);

    ast.context.values.insert("TOPDIR", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("/user/proj/android/korhal/")));
    ast.context.values.insert("TOP", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("/user/proj/android/korhal/")));
    ast.context.values.insert("TARGET_DEVICE", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("maguro")));
    ast.context.values.insert("TARGET_ARCH", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("arm")));
    ast.context.values.insert("TARGET_ARCH_VARIANT", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("rmv7-a")));
    ast.context.values.insert("TARGET_OS", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("linux")));
    ast.context.values.insert("TARGET_EXECUTABLE_SUFFIX", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("")));
    ast.context.values.insert("TARGET_OUT_STATIC_LIBRARIES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("hurz")));
    ast.context.values.insert("TARGET_OUT_SHARED_LIBRARIES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("murf")));
    ast.context.values.insert("TARGET_OUT", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("durr")));
    ast.context.values.insert("TARGET_OUT_DATA", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("blorp")));
    ast.context.values.insert("TARGET_OUT_ETC", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("etc")));
    ast.context.values.insert("TARGET_OUT_DATA_APPS", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("barfwlp")));
    ast.context.values.insert("TARGET_OUT_COMMON_INTERMEDIATES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("askd")));
    ast.context.values.insert("TARGET_OUT_OPTIONAL_EXECUTABLES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("ooop")));
    ast.context.values.insert("TARGET_OUT_EXECUTABLES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("nonom")));
    ast.context.values.insert("TARGET_OUT_INTERMEDIATE_LIBRARIES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("barz")));
    ast.context.values.insert("TARGET_ROOT_OUT", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("moausd")));
    ast.context.values.insert("HOST_OUT", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("wurbel")));
    ast.context.values.insert("HOST_OS", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("linux")));
    ast.context.values.insert("HOST_ARCH", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("x86")));
    ast.context.values.insert("HOST_JDK_TOOLS_JAR", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("hirmlml")));
    ast.context.values.insert("HOST_EXECUTABLE_SUFFIX", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("")));
    ast.context.values.insert("HOST_EXECUTABLES_SUFFIX", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("")));
    ast.context.values.insert("HOST_OUT_EXECUTABLES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("blaarfwas")));
    ast.context.values.insert("HOST_OUT_JAVA_LIBRARIES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("somewhereovertherainbow")));
    ast.context.values.insert("PRODUCT_OUT", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("quawarf")));
    ast.context.values.insert("BUILD_OUT_EXECUTABLES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("zxccz")));
    ast.context.values.insert("BUILD_EXECUTABLE_SUFFIX", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("")));
    ast.context.values.insert("common_SHARED_LIBRARIES", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("")));
    ast.context.values.insert("KERNEL_HEADERS", QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression("")));


    //special cases. belong in config

    QMap<QByteArray,QByteArray> special;
    special["space"]=" ";
    special["full_target"]="";
    special["WITH_MALLOC_CHECK_LIBC_A"]="";
    special["BUILD_SYSTEM"]="";
    special["BOARD_WPA_SUPPLICANT_DRIVER"]="";
    special["BOARD_WPA_SUPPLICANT_PRIVATE_LIB"]="";
    special["BOARD_HOSTAPD_DRIVER"]="";
    special["BOARD_HOSTAPD_PRIVATE_LIB"]="";
    special["DRV_LDFLAGS"]="";
    special["DRV_WPA_LDFLAGS"]="";
    special["DRV_AP_LDFLAGS"]="";
    special["PLATFORMSDKLIB"]="";
    special["HISTORICAL_NDK_VERSIONS_ROOT"]="";
    special["LOCAL_NDK_VERSION"]="";
    special["LOCAL_SDK_VERSION"]="";
    special["DEBUG_DALVIK_VM"]="";
    special["WITH_COPYING_GC"]="";
    special["WITH_JIT"]="";
    special["JNI_H_INCLUDE"]="";
    special["CLANG_CONFIG_UNKNOWN_CFLAGS"]="";
    special["TARGET_PREBUILT_TAG"]="";
    special["HOST_PREBUILT_TAG"]="";
    special["FRAMEWORKS_BASE_SUBDIRS"]="";
    special["SYSROOT"]="";
    special["TOOL_LDFLAGS"]="";
    special["log_c_includes"]="";
    special["log_shared_libraries"]="";
    special["libportable_common_src_files"]="";
    special["local_javac_flags"]="";
    special["LOCAL_INSTALLED_MODULE"]="";
    special["TARGET_RECOVERY_UI_LIB"]="";
    special["TARGET_DISK_CONFIG_LIB"]="";
    special["SRC_TARGET_DIR"]="";
    special["LOCAL_BUILT_MODULE"]="";
    special["TARGET_RECOVERY_UPDATER_LIBS"]="";
    special["TARGET_RECOVERY_UPDATER_EXTRA_LIBS"]="";
    special["YACC"]="yacc";
    special["common_cxxflags"]="";
    special["so_suffix"]=".so";
    special["LOCAL_C_INCLUDES"]="";
    special["LOCAL_CFLAGS"]="";
    special["OUT_DOCS"]="";
    special["TARGET_AVAILABLE_SDK_VERSIONS"]="";
    special["TARGET_ROOT_OUT_SBIN"]="";
    special["TARGET_ROOT_OUT_SBIN_UNSTRIPPED"]="";
    special["COMMON_JAVA_PACKAGE_SUFFIX"]="";
    special["FRAMEWORKS_BASE_JAVA_SRC_DIRS"]="";
    special["SRC_API_DIR"]="";
    special["LLVM_HOST_BUILD_MK"]="/dev/zero";
    special["CLEAR_TBLGEN_VARS"]="/dev/zero";
    special["LOCAL_CPPFLAGS"]="";
    special["ALL_MODULES.clang.INSTALLED"]="";
    special["CLANG_CXX"]="";
    special["HOST_CC"]="gcc";
    special["HOST_CXX"]="g++";
    special["HOST_AR"]="ar";
    special["dir"]="";
    special["1"]="";
    special["intermediates"]="";
    special["FRAMEWORKS_SUPPORT_JAVA_SRC_DIRS"]="";
    special["INTERNAL_PLATFORM_API_FILE"]="";
    special["out_dir"]="";
    special["framework_docs_LOCAL_STATIC_JAVA_LIBRARIES"]="";
    special["intermediates.COMMON"]="";
    special["MY_lib_sources"]="";
    special["PLATFORM_VERSION"]="4";
    special["LOCAL_SHARED_LIBRARIES"]="";
    special["LOCAL_STATIC_LIBRARIES"]="";
    special["LOCAL_MODULE"]="";
    special["cacert"]="";
    special["ALL_MODULES"]="";
    special["ALL_MODULE_TAGS"]="";
    special["ALL_MODULE_NAME_TAGS.tests"]="";
    special["ASR_WORKAROUND_DEFINES"]="";
    special["ACP"]="";
    special["QEMU_SDL_CONFIG"]="";
    special["ZLIB_CFLAGS"]="";
    special["EMULATOR_COMMON_LDLIBS"]="";
    special["ANDROID_BUILD_SHELL"]="";



    foreach (const QByteArray &key, special.keys()) {
        ast.context.values.insert(key, QList<Expression::Ptr>() << Expression::Ptr(new LiteralExpression(special.value(key))));
    }


    Node::Ptr n = ast(&lex);

    if (!n)
        return 2;
    MakefileSemantics sem(ast);
    sem(n);

    return 0;
}
