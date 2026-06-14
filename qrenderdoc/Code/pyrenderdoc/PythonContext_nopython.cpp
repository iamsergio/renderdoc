/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2017-2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

// Stub used when ENABLE_PYRENDERDOC=OFF. Provides no-op implementations of
// all PythonContext methods without requiring Python.h or SWIG-generated code.
// PyObject is safe to use as an opaque pointer type via the forward declaration
// in PythonContext.h ("typedef struct _object PyObject").

#include <QMap>
#include <QStringList>
#include <QWidget>
#include <functional>
#include "Code/Interface/QRDInterface.h"
#include "PythonContext.h"

// Static member definitions
PyObject *PythonContext::main_dict = nullptr;
QMap<rdcstr, PyObject *> PythonContext::extensions;

PythonContext::PythonContext(QObject *parent) : QObject(parent)
{
}

PythonContext::~PythonContext()
{
}

void PythonContext::Finish()
{
  delete this;
}

void PythonContext::GlobalInit()
{
}

void PythonContext::GlobalShutdown()
{
}

QStringList PythonContext::GetApplicationExtensionsPaths()
{
  return {};
}

void PythonContext::ProcessExtensionWork(std::function<void()> callback)
{
  callback();
}

QString PythonContext::LoadExtension(ICaptureContext &ctx, const rdcstr &extension)
{
  return lit("Python scripting is not available in this build");
}

void PythonContext::ConvertPyArgs(const ExtensionCallbackData &data,
                                  rdcarray<rdcpair<rdcstr, PyObject *>> &args)
{
}

void PythonContext::FreePyArgs(rdcarray<rdcpair<rdcstr, PyObject *>> &args)
{
}

bool PythonContext::CheckInterfaces(rdcstr &log)
{
  return true;
}

QString PythonContext::versionString()
{
  return lit("Python scripting disabled");
}

QStringList PythonContext::completionOptions(QString base)
{
  return {};
}

void PythonContext::executeString(const QString &source)
{
}

void PythonContext::executeString(const QString &filename, const QString &source)
{
}

void PythonContext::executeFile(const QString &filename)
{
}

void PythonContext::setGlobal(const char *varName, const char *typeName, void *object)
{
}

void PythonContext::setPyGlobal(const char *varName, PyObject *object)
{
}

void PythonContext::PausePythonThreading()
{
}

void PythonContext::ResumePythonThreading()
{
}

// Called by the inline QWidgetToPy() in PythonContext.h
PyObject *PythonContext::QtObjectToPython(const char *typeName, QObject *object)
{
  return nullptr;
}

QWidget *PythonContext::QWidgetFromPy(PyObject *widget)
{
  return nullptr;
}

// Template specialisations declared in PythonContext.h
template <>
void PythonContext::setGlobal(const char *varName, PyObject *object)
{
}

template <>
void PythonContext::setGlobal(const char *varName, QObject *object)
{
}

template <>
void PythonContext::setGlobal(const char *varName, QWidget *object)
{
}
