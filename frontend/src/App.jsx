import { useEffect, useState } from 'react';
import { listStudents, createStudent, updateStudent, deleteStudent } from './api.js';
import StudentsGrid from './components/StudentsGrid.jsx';
import StudentForm from './components/StudentForm.jsx';
import ConfirmDialog from './components/ConfirmDialog.jsx';

export default function App() {
  const [students, setStudents] = useState([]);
  const [loading, setLoading] = useState(true);
  const [networkError, setNetworkError] = useState(null);
  const [editing, setEditing] = useState(null);
  const [showForm, setShowForm] = useState(false);
  const [confirmTarget, setConfirmTarget] = useState(null);

  async function refresh() {
    setLoading(true);
    setNetworkError(null);
    try {
      const data = await listStudents();
      setStudents(data);
    } catch (err) {
      setNetworkError(err.message || 'No se puede conectar al servidor');
    } finally {
      setLoading(false);
    }
  }

  useEffect(() => { refresh(); }, []);

  function openCreate() {
    setEditing(null);
    setShowForm(true);
  }

  function openEdit(student) {
    setEditing(student);
    setShowForm(true);
  }

  async function handleSubmit(data) {
    if (editing) {
      await updateStudent(editing.id, data);
    } else {
      await createStudent(data);
    }
    setShowForm(false);
    setEditing(null);
    await refresh();
  }

  function askDelete(student) {
    setConfirmTarget(student);
  }

  async function confirmDelete() {
    if (!confirmTarget) return;
    try {
      await deleteStudent(confirmTarget.id);
      setConfirmTarget(null);
      await refresh();
    } catch (err) {
      setNetworkError(err.message || 'Error al eliminar');
      setConfirmTarget(null);
    }
  }

  return (
    <div className="app">
      <header className="topbar">
        <h1>Estudiantes</h1>
        <button className="btn-primary" onClick={openCreate}>+ Nuevo</button>
      </header>

      {networkError && (
        <div className="banner-error">
          {networkError === 'Failed to fetch'
            ? 'No se puede conectar al servidor'
            : networkError}
        </div>
      )}

      {loading ? (
        <p className="muted">Cargando…</p>
      ) : (
        <StudentsGrid
          students={students}
          onEdit={openEdit}
          onDelete={askDelete}
        />
      )}

      {showForm && (
        <StudentForm
          initial={editing}
          onCancel={() => { setShowForm(false); setEditing(null); }}
          onSubmit={handleSubmit}
        />
      )}

      {confirmTarget && (
        <ConfirmDialog
          message={`¿Eliminar a ${confirmTarget.name}?`}
          onCancel={() => setConfirmTarget(null)}
          onConfirm={confirmDelete}
        />
      )}
    </div>
  );
}
