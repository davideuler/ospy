//
// Copyright (c) 2007 Ole Andr� Vadla Ravn�s <oleavr@gmail.com>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//

#include "stdafx.h"
#include "BinaryLogger.h"

BinaryLogger::BinaryLogger(const OString &filename)
    : m_id(0)
{
	m_handle = CreateFile(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_handle == INVALID_HANDLE_VALUE)
		throw runtime_error("CreateFile failed");
}

BinaryLogger::~BinaryLogger()
{
	CloseHandle(m_handle);
}

Logging::Event *
BinaryLogger::NewEvent(const OString &eventType)
{
    // TODO: have m_id in shared memory so everything that gets logged
    //       shares the same namespace.
    return new Logging::Event(this, m_id++, eventType);
}

void
BinaryLogger::SubmitEvent(Logging::Event *ev)
{
	BinarySerializer serializer;

	serializer.AppendNode(ev);

	const OString &buf = serializer.GetData();

	DWORD bytesWritten;
	if (!WriteFile(m_handle, buf.data(), static_cast<DWORD>(buf.size()), &bytesWritten, NULL))
		throw runtime_error("WriteFile failed");

	if (bytesWritten != buf.size())
		throw runtime_error("short write");
}

void
BinarySerializer::AppendNode(Logging::Node *node)
{
	// Name
	AppendString(node->GetName());

	// Fields
	{
		AppendDWord(node->GetFieldCount());
		Logging::Node::FieldListConstIter iter, endIter = node->FieldsIterEnd();

		for (iter = node->FieldsIterBegin(); iter != endIter; iter++)
		{
			AppendString(iter->first);
			AppendString(iter->second);
		}
	}

    // Content
    AppendDWord(node->GetContentIsRaw());
    AppendString(node->GetContent());

	// Children
	{
		AppendDWord(node->GetChildCount());
		Logging::Node::ChildListConstIter iter, endIter = node->ChildrenIterEnd();

		for (iter = node->ChildrenIterBegin(); iter != endIter; iter++)
		{
			AppendNode(*iter);
		}
	}
}

void
BinarySerializer::AppendString(const OString &s)
{
	AppendDWord(static_cast<DWORD>(s.size()));
    if (s.size() > 0)
	    m_buf.append(s.data(), s.size());
}

void
BinarySerializer::AppendDWord(DWORD dw)
{
	m_buf.append(reinterpret_cast<const char *>(&dw), sizeof(dw));
}
