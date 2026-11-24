# QueueOS Security Model

## 1. Purpose

This document describes the security architecture of QueueOS, including application authorization, access to operating-system functionality, and the protection of system resources and data.

Security is a foundational part of the OS architecture, not an additional layer that applications are expected to implement independently.

QueueOS should provide a consistent security model across applications and operating-system services, with authorization decisions enforced at the appropriate system boundaries.

A central distinction in this model is between **Features** and **Rights**:

- **Features** describe what an application provides or supports.
- **Rights** describe what an application is authorized to access or do.

These concepts must remain separate. An application's functionality does not automatically grant it permission to use every OS facility that might be useful to that functionality.

## 2. Security Principles

The security architecture should follow these principles:

- **Explicit authorization:** Applications receive only the Rights necessary for their intended operation.
- **User awareness:** Users should be able to understand what access an application is requesting and what granting that access means.
- **Enforcement by the OS:** Authorization must be enforced by the operating system, not merely declared by applications.
- **Consistent policy:** Applications and services should use a common authorization model.
- **Isolation:** Applications should not gain access to another application's data or resources without authorization.
- **Least privilege:** Applications should not receive unrestricted access merely for implementation convenience.
- **Revocability:** Granted Rights should be capable of being withdrawn.
- **Auditability:** Authorization decisions and security-relevant actions should be inspectable.
- **Minimal trusted infrastructure:** The components responsible for enforcing security should be kept as small and understandable as practical.

## 3. Features and Rights

Features and Rights serve different purposes.

### 3.1 Features

Features describe the functionality an application offers.

Examples might include:

- Email client.
- Calendar.
- Image editor.
- File manager.
- Database client.
- Network monitoring.
- Audio playback.

Features are descriptive. They help the OS and user understand what an application does, but they do not independently authorize access to resources.

An application declaring itself to be a file manager does not automatically receive unrestricted filesystem access.

### 3.2 Rights

Rights describe the permissions an application has been granted.

Examples might include:

- Read files in an authorized location.
- Write files in an authorized location.
- Access network services.
- Use a camera.
- Access a microphone.
- Display notifications.
- Communicate with another application.
- Access a particular device.
- Use a specific OS service.

Rights should be sufficiently specific to avoid granting more access than necessary.

Where appropriate, a Right should also identify the scope of access. Permission to read one selected file should not imply permission to read every file on the system.

### 3.3 Relationship between Features and Rights

Features may imply that an application will request particular Rights, but they must not grant those Rights automatically.

For example, an image editor may offer an import feature. It might request permission to read user-selected files. It should not automatically receive unrestricted access to the entire storage system.

Similarly, an application may advertise network functionality while requesting access only to a particular service or resource.

The user should be able to distinguish what an application can do from what it is asking permission to access.

## 4. Application Installation and Authorization

Application installation is an important security boundary.

When an application is installed, QueueOS should present an authorization panel describing the Rights requested by the application.

This panel should appear as part of the installation process, before the application is allowed to exercise the requested permissions.

The objective is to make authorization understandable and explicit rather than hiding it in a technical manifest or an obscure settings page.

### 4.1 Authorization panel

The panel should identify:

- The application.
- Its declared Features.
- The Rights it is requesting.
- A meaningful explanation of each requested Right.
- The scope of each permission.
- Which permissions are required for the application to function.
- Which permissions are optional.
- Any particularly sensitive access.

The presentation should make it clear what the user is authorizing.

The interface should avoid presenting a long list of unexplained technical permission identifiers.

### 4.2 Granting Rights

The user should be able to review the requested Rights and decide whether to grant them.

The system should distinguish between permissions that are necessary for the application's declared functionality and permissions that enable optional functionality.

Where practical, users should be able to install an application without granting every optional Right.

An application must handle denied permissions appropriately. It should not assume that every requested Right will be granted.

### 4.3 Changing authorization later

Authorization should not be a one-time decision that becomes permanent.

Users should be able to inspect and modify an application's granted Rights after installation.

The OS should enforce changes to authorization without relying on the application to voluntarily stop using a permission that has been revoked.

## 5. Authorization Enforcement

The OS must enforce Rights at the point where protected functionality is accessed.

An application declaring a Right in its manifest does not establish that it possesses that Right.

Likewise, an application holding a reference to a queue or service must not automatically be considered authorized to perform every operation exposed by that service.

Authorization checks should occur at the appropriate service boundary.

For example:

1. An application submits a request through a queue.
2. The receiving service identifies the requesting application or security principal.
3. The service determines whether the requested operation is permitted.
4. The operation is allowed or rejected.
5. The result is returned through the appropriate interface.

The precise enforcement mechanism remains an implementation decision. The important requirement is that applications cannot bypass authorization simply by constructing a different request or accessing a lower-level interface.

### 5.1 Identity

The OS needs a reliable way to identify the security principal responsible for an operation.

An application must not be able to impersonate another application merely by supplying its identifier in a request.

The identity mechanism must remain trustworthy across queue operations, service boundaries, and asynchronous execution.

The precise representation of application identities and their relationship to installed application instances remains to be designed.

### 5.2 Rights propagation

An operation may pass through multiple OS components before completion.

The security model must preserve the authorization context throughout that execution.

A service should not be able to use its own authority to perform an operation on behalf of an application that lacks the necessary Right, unless an explicitly defined delegation mechanism permits it.

This is particularly important for services that act as intermediaries between applications and protected resources.

## 6. Scope and Granularity

Rights should be scoped as narrowly as practical.

Possible scopes include:

- A particular file or object.
- A directory or storage namespace.
- A network endpoint or service.
- A particular device.
- A specific OS service.
- A defined category of operations.

The architecture should avoid treating permissions as unnecessarily broad global switches.

For example, permission to access a particular selected file should not automatically authorize access to unrelated files.

The exact granularity will depend on the resource and the service exposing it.

There may be cases where a broader Right is necessary, but the scope should be explicit and understandable.

## 7. Application Isolation

Applications should not be able to access one another's private resources by default.

The OS must provide isolation for:

- Application data.
- Memory and execution resources.
- Queue endpoints.
- Application-specific configuration.
- Credentials and authentication material.
- Other protected resources.

Isolation should be enforced by the OS rather than relying on applications to respect conventions.

Applications may communicate with one another through explicitly authorized interfaces.

The queue architecture provides a natural mechanism for such communication, but the existence of a queue does not imply that every application can access it.

The system must define how applications discover and request access to published services and how that access is authorized.

## 8. Data Protection

The OS Database is a central component of QueueOS and may contain sensitive system and application information.

The security model must protect data according to ownership and authorization.

This includes:

- Application-specific data.
- System configuration.
- User information.
- Authentication credentials.
- Security policies and authorization records.
- Application installation metadata.
- Persistent service state.

The database must not become a mechanism through which applications can bypass access controls by querying internal tables or storage structures directly.

Applications should interact with database functionality through authorized interfaces.

The exact database encryption, key management, and data-at-rest protection strategies remain open design questions.

## 9. Application Lifecycle and Revocation

Security decisions must remain valid throughout an application's lifecycle.

Relevant lifecycle events include:

- Installation.
- Initial authorization.
- Application updates.
- Permission changes.
- Application suspension.
- Application termination.
- Uninstallation.

When a Right is revoked, the OS must prevent further use of that Right.

The implementation must also consider operations already in progress.

For example, revoking permission to access a resource should prevent new unauthorized operations. Whether an operation already underway is immediately cancelled, allowed to finish, or handled according to a service-specific policy must be defined.

Uninstallation should remove the application's authorization records and invalidate access associated with the installed application instance.

The handling of retained user data and application-owned persistent resources should be explicit rather than assumed.

## 10. Service Security

OS services are responsible for enforcing the authorization requirements of the functionality they expose.

A service should not assume that every request arriving through a valid queue is legitimate.

It must consider:

- The identity of the requesting application.
- The requested operation.
- The scope of the requested resource.
- The application's granted Rights.
- The validity of supplied data.
- Resource limits and abuse conditions.

Services should expose only the operations necessary for their intended purpose.

Internal service interfaces should not automatically become application-facing interfaces.

The architecture should make it possible to keep sensitive implementation details private while exposing well-defined, authorized functionality.

## 11. Trusted Computing Base

The Trusted Computing Base (TCB) consists of the components that must behave correctly for the security model to be effective.

At a minimum, it will include the mechanisms responsible for:

- Establishing application identity.
- Maintaining authorization records.
- Enforcing Rights.
- Protecting memory and execution boundaries.
- Controlling access to protected resources.
- Validating security-sensitive operations.

The exact TCB boundaries depend on the final execution and isolation architecture.

The goal should be to minimize the amount of code that must be trusted, particularly code that handles application requests or controls access to sensitive resources.

A large collection of independently implemented permission checks would make the system difficult to reason about and audit.

## 12. Security and the OS Database

The OS Database stores information that is essential to the operation and integrity of QueueOS.

Security-related records may include:

- Installed application identities.
- Declared application Features.
- Requested Rights.
- Granted and denied Rights.
- Authorization scope.
- Application ownership.
- Security policy.
- Relevant audit information.

The database should provide a consistent representation of these records.

However, storing an authorization record in the database is not itself an enforcement mechanism.

The OS must ensure that the authoritative authorization state is consulted or otherwise reliably enforced when protected operations occur.

Database recovery and generation changes must also preserve the integrity of security-related state.

The exact schema and relationship between authorization records and application installation metadata will be defined separately.

## 13. Auditing and Transparency

Users and administrators should be able to understand which applications have access to which resources.

QueueOS should provide a way to inspect:

- Installed applications.
- Declared Features.
- Requested Rights.
- Granted Rights.
- Denied Rights.
- Changes to authorization.
- Security-relevant events.

The system should also consider recording significant authorization failures and other security events.

Audit records must not expose sensitive information unnecessarily, and the mechanism should not become a source of unbounded storage consumption.

The precise audit format, retention policy, and presentation remain open.

## 14. Updates and Trust

Application updates introduce additional security considerations.

An update may change an application's functionality or introduce new requested Rights.

An application should not silently acquire additional permissions merely because an update has been installed.

Where an update requests new Rights, the OS should make that change visible and obtain authorization as appropriate.

The system will also need a way to establish the authenticity and integrity of application packages.

Package signing, publisher identity, trust management, and update verification are important areas for further design.

The precise trust model has not yet been established.

## 15. Threat Model

The security architecture should account for at least the following threats:

- An application attempting to access resources for which it has no Right.
- An application attempting to impersonate another application.
- An application attempting to access another application's private data.
- A malicious or compromised service abusing its authority.
- Malformed requests intended to exploit queue or service implementations.
- Resource exhaustion through excessive submissions or allocations.
- Attempts to bypass authorization through alternate interfaces.
- Attempts to retain access after a Right has been revoked.
- Tampering with application metadata or authorization records.
- Malicious or compromised application updates.

The system should also consider failures in trusted OS components.

No authorization model can guarantee security if its enforcement mechanisms can be arbitrarily bypassed by compromised components with equivalent authority.

The architecture should make these trust boundaries explicit.

## 16. Open Design Questions

The following questions require further investigation.

### Identity and authorization

- How are application identities generated and maintained?
- Are Rights assigned to application identities, individual installations, users, or some combination?
- How are application updates associated with existing authorization records?
- Can applications delegate Rights to other applications?
- How is authorization context preserved across asynchronous operations?

### Rights model

- What is the canonical representation of a Right?
- How are scopes represented?
- Which Rights are global, and which are resource-specific?
- Can a Right be granted temporarily?
- Can a service request authorization at runtime?

### Installation and updates

- What package metadata declares Features and requested Rights?
- How are required and optional Rights distinguished?
- What happens when an update requests additional Rights?
- How are application packages authenticated and verified?

### Enforcement

- Which component maintains authoritative authorization state?
- How are checks performed efficiently?
- How are revocations propagated to active services and outstanding operations?
- What isolation mechanisms are available to applications?

### Data protection

- How are application-owned database records isolated?
- What encryption and key-management mechanisms are required?
- How are credentials protected?
- What audit information should be retained?

These questions should be addressed incrementally, with particular attention to keeping the enforcement model consistent and understandable.

## 17. Current Direction and Commitments

### Established direction

- Features and Rights are separate concepts.
- Features describe application functionality.
- Rights describe what an application is authorized to access or do.
- Application installation should present an authorization panel describing requested Rights.
- Users should be able to review and grant permissions.
- Rights should be enforceable by the OS.
- Authorization should be revocable.
- Queue access does not automatically imply authorization for every operation exposed by a service.
- Applications should not receive unrestricted access by default.

### Not yet established

- The exact identity model.
- The canonical representation of Rights.
- The granularity of permission scopes.
- The precise authorization enforcement mechanism.
- Whether authorization is checked centrally, by individual services, or through a combination.
- The isolation mechanisms used for applications.
- The package-signing and publisher-trust model.
- Database encryption and key management.
- Audit record format and retention.
- Runtime permission-request behaviour.

Implementation decisions should preserve the established principles while treating unresolved mechanisms as design work.

---

**Status:** Security principles and authorization direction established; enforcement mechanisms under design.

**Related documents:**
- `ARCHITECTURE.md`
- `DATABASE.md`
- `EXECUTION.md`
- `STORAGE.md`
- `AGENTS.md`
