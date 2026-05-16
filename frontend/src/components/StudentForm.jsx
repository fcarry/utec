import { useState } from 'react';

function validate({ name, id_doc, email }) {
  const errors = {};
  if (!name || name.length < 1 || name.length > 100) {
    errors.name = 'Nombre requerido (1-100 caracteres).';
  }
  if (!/^\d{7,10}$/.test(id_doc || '')) {
    errors.id_doc = 'Cédula debe tener 7-10 dígitos.';
  }
  if (!email || email.length > 120) {
    errors.email = 'Email requerido (máx. 120 caracteres).';
  } else {
    const at = email.indexOf('@');
    const dot = email.indexOf('.', at);
    if (at < 1 || dot < at + 2 || dot === email.length - 1) {
      errors.email = 'Email inválido.';
    }
  }
  return errors;
}

export default function StudentForm({ initial, onCancel, onSubmit }) {
  const [form, setForm] = useState({
    name: initial?.name ?? '',
    id_doc: initial?.id_doc ?? '',
    email: initial?.email ?? '',
  });
  const [errors, setErrors] = useState({});
  const [serverError, setServerError] = useState(null);
  const [submitting, setSubmitting] = useState(false);

  function update(field, value) {
    setForm((f) => ({ ...f, [field]: value }));
  }

  async function handleSubmit(e) {
    e.preventDefault();
    setServerError(null);
    const errs = validate(form);
    setErrors(errs);
    if (Object.keys(errs).length > 0) return;
    setSubmitting(true);
    try {
      await onSubmit(form);
    } catch (err) {
      if (err.status === 409) {
        setServerError('Ya existe un estudiante con esa cédula.');
      } else {
        setServerError(err.message || 'Error al guardar');
      }
    } finally {
      setSubmitting(false);
    }
  }

  return (
    <div className="modal-backdrop" onClick={onCancel}>
      <div className="modal" onClick={(e) => e.stopPropagation()}>
        <h2>{initial ? 'Editar estudiante' : 'Nuevo estudiante'}</h2>
        <form onSubmit={handleSubmit}>
          <label>
            Nombre
            <input
              type="text"
              value={form.name}
              maxLength={100}
              onChange={(e) => update('name', e.target.value)}
              autoFocus
            />
            {errors.name && <span className="field-error">{errors.name}</span>}
          </label>
          <label>
            Cédula
            <input
              type="text"
              value={form.id_doc}
              maxLength={10}
              inputMode="numeric"
              onChange={(e) => update('id_doc', e.target.value)}
            />
            {errors.id_doc && <span className="field-error">{errors.id_doc}</span>}
          </label>
          <label>
            Email
            <input
              type="email"
              value={form.email}
              maxLength={120}
              onChange={(e) => update('email', e.target.value)}
            />
            {errors.email && <span className="field-error">{errors.email}</span>}
          </label>
          {serverError && <div className="banner-error inline">{serverError}</div>}
          <div className="modal-actions">
            <button type="button" className="btn-ghost" onClick={onCancel} disabled={submitting}>
              Cancelar
            </button>
            <button type="submit" className="btn-primary" disabled={submitting}>
              {submitting ? 'Guardando…' : 'Guardar'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
