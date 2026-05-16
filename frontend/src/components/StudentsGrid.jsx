export default function StudentsGrid({ students, onEdit, onDelete }) {
  if (students.length === 0) {
    return <p className="muted">No hay estudiantes cargados.</p>;
  }
  return (
    <table className="grid">
      <thead>
        <tr>
          <th>ID</th>
          <th>Nombre</th>
          <th>Cédula</th>
          <th>Email</th>
          <th className="col-actions">Acciones</th>
        </tr>
      </thead>
      <tbody>
        {students.map((s) => (
          <tr key={s.id}>
            <td>{s.id}</td>
            <td>{s.name}</td>
            <td>{s.id_doc}</td>
            <td>{s.email}</td>
            <td className="col-actions">
              <button
                className="btn-icon"
                title="Editar"
                onClick={() => onEdit(s)}
              >✎</button>
              <button
                className="btn-icon btn-danger"
                title="Eliminar"
                onClick={() => onDelete(s)}
              >🗑</button>
            </td>
          </tr>
        ))}
      </tbody>
    </table>
  );
}
