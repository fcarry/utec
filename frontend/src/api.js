const BASE = import.meta.env.VITE_API_BASE ?? 'http://localhost:8080';

async function parseJsonOrError(res) {
  const text = await res.text();
  let body = null;
  if (text) {
    try { body = JSON.parse(text); } catch { body = null; }
  }
  if (!res.ok) {
    const message = body?.error || `HTTP ${res.status}`;
    const err = new Error(message);
    err.status = res.status;
    throw err;
  }
  return body;
}

export const listStudents = () =>
  fetch(`${BASE}/students`).then(parseJsonOrError);

export const createStudent = (data) =>
  fetch(`${BASE}/students`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data),
  }).then(parseJsonOrError);

export const updateStudent = (id, data) =>
  fetch(`${BASE}/students/${id}`, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data),
  }).then(parseJsonOrError);

export const deleteStudent = (id) =>
  fetch(`${BASE}/students/${id}`, { method: 'DELETE' }).then(parseJsonOrError);
