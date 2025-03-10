#!/bin/sh

# This script is used by various build projects on Jenkins

# See https://ci.inria.fr/simgrid/job/SimGrid-Multi/configure
# See https://ci.inria.fr/simgrid/job/Simgrid-Windows/configure

set -e

# Cleanup previous attempts
rm -rf /tmp/simgrid*tmp
rm -rf /tmp/surf-java*tmp

# Help older cmakes
if [ -e /usr/lib/jvm/java-7-openjdk-amd64 ] ; 
then
  export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
fi

# usage: die status message...
die () {
  local status=${1:-1}
  shift
  [ $# -gt 0 ] || set -- "Error - Halting"
  echo "$@" >&2
  exit $status
}

# Get an ON/OFF string from a command:
onoff() {
  if "$@" > /dev/null ; then
    echo ON
  else
    echo OFF
  fi
}

build_mode="$1"
echo "Build mode $build_mode on $(uname -np)" >&2
case "$build_mode" in
  "Debug")
  ;;

  "ModelChecker")
  ;;

  "DynamicAnalysis")
  ;;

  *)
    die 1 "Unknown build_mode $build_mode"
  ;;
esac

if test "$(uname -o)" = "Msys"; then
  if [ -z "$NUMBER_OF_PROCESSORS" ]; then
    NUMBER_OF_PROCESSORS=1
  fi
  GENERATOR="MSYS Makefiles"
else
  NUMBER_OF_PROCESSORS="$(nproc)" || NUMBER_OF_PROCESSORS=1
  GENERATOR="Unix Makefiles"
fi

ulimit -c 0 || true

if test "$(uname -o)" != "Msys"; then
  echo "XX"
  echo "XX Get out of the tree"
  echo "XX"
  if [ -d $WORKSPACE/build ]
  then
    rm -rf $WORKSPACE/build
  fi
  mkdir $WORKSPACE/build
  cd $WORKSPACE/build

  echo "XX"
  echo "XX Build the archive out of the tree"
  echo "XX   pwd: `pwd`"
  echo "XX"

  cmake -G"$GENERATOR" -Denable_documentation=OFF $WORKSPACE
  make dist -j$NUMBER_OF_PROCESSORS

  echo "XX"
  echo "XX Open the resulting archive"
  echo "XX"
  tar xzf `cat VERSION`.tar.gz
  cd `cat VERSION`
fi

echo "XX"
echo "XX Configure and build SimGrid"
echo "XX   pwd: `pwd`"
echo "XX"
cmake -G"$GENERATOR"\
  -Denable_debug=ON -Denable_documentation=OFF -Denable_coverage=OFF \
  -Denable_model-checking=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_smpi_ISP_testsuite=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_compile_optimizations=$(onoff test "$build_mode" = "Debug") \
  -Denable_smpi_MPICH3_testsuite=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_lua=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_mallocators=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_memcheck=$(onoff test "$build_mode" = "DynamicAnalysis") \
  -Denable_compile_warnings=ON -Denable_smpi=ON -Denable_lib_static=OFF \
  -Denable_latency_bound_tracking=OFF -Denable_jedule=OFF \
  -Denable_tracing=ON -Denable_java=ON
make -j$NUMBER_OF_PROCESSORS VERBOSE=1

if test "$(uname -o)" != "Msys"; then
  cd $WORKSPACE/build
  cd `cat VERSION`
fi

TRES=0

echo "XX"
echo "XX Run the tests"
echo "XX   pwd: `pwd`"
echo "XX"

ctest -T test --output-on-failure --no-compress-output || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/tools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults.xml
   mv CTestResults.xml $WORKSPACE
fi

echo "XX"
echo "XX Done. Return the results to cmake"
echo "XX"

if [ "$build_mode" = "DynamicAnalysis" ]
then
  ctest -D ContinuousStart
  ctest -D ContinuousConfigure
  ctest -D ContinuousBuild
  ctest -D ContinuousMemCheck
  ctest -D ContinuousSubmit
fi

ctest -D ContinuousStart
ctest -D ContinuousConfigure
ctest -D ContinuousBuild
ctest -D ContinuousTest
ctest -D ContinuousSubmit

rm -rf `cat VERSION`
