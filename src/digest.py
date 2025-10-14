# digest.py
# <legal>
# Pointer Ownership Model (POM) Source Code Release
# 
# Copyright 2025 Carnegie Mellon University.
# 
# NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
# INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
# UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
# IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
# FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
# OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
# MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
# TRADEMARK, OR COPYRIGHT INFRINGEMENT.
# 
# Licensed under a MIT (SEI)-style license, please see license.txt or
# contact permission@sei.cmu.edu for full terms.
# 
# [DISTRIBUTION STATEMENT A] This material has been approved for public
# release and unlimited distribution.  Please see Copyright notice for
# non-US Government use and distribution.
# 
# DM25-1262
# </legal>


from typing import Optional, List, OrderedDict, Any
from collections import OrderedDict

"""
Class hierarchy:
- ASTNode
    - Decl
        - NamedDecl
            - ValueDecl
                - FunctionDecl
                - VarDecl
                    - ParmVarDecl
            - TypeDecl
                - TypedefNameDecl
                    - TypedefDecl
    - Stmt
        - ControlFlowStmt
            - IfStmt
            - WhileStmt
            - ForStmt
            - SwitchStmt
                - CaseStmt
                - DefaultStmt
            - DoStmt
            - BreakStmt
            - ContinueStmt
            - ReturnStmt
            - GotoStmt
            - CompoundStmt
        - DeclStmt
        - ValueStmt
            - LabelStmt
            - Expr
                - Operator
                    - BinaryOperator
                    - UnaryOperator
                - CallExpr
                - LiteralExpr
                    - IntegerLiteral
                    - FloatingLiteral
                    - CharacterLiteral
                    - StringLiteral
                - DeclRefExpr
                - CastExpr
                    - ImplicitCastExpr
"""


class ASTNode:
    """
    Base class for all AST nodes.

    Attributes:
        kind (str): The 'kind' of the AST node.
        id (Optional[str]): The unique identifier of the node.
        loc (Optional[OrderedDict]): The location of the node in source.
        range (Optional[OrderedDict]): The range of the node in source.
        children (List['ASTNode']): List of child AST nodes.
        raw (OrderedDict): The raw AST node.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        self.kind = kind
        self.id: Optional[str] = raw.get("id")
        self.loc: Optional[OrderedDict] = raw.get("loc")
        self.range: Optional[OrderedDict] = raw.get("range")
        self.children: List['ASTNode'] = []
        self.raw = raw

        if self.id:
            self.id = str(self.id)

        # process children
        for child in self.raw.get("inner", []):
            cnode = digest(child)
            if cnode:
                self.add_child(cnode)

    def add_child(self, child: 'ASTNode') -> None:
        self.children.append(child)

    def walk_children(self):
        generator = (child for child in self.children)
        yield next(generator)

    @staticmethod
    def find_by_id(node: 'ASTNode', id: str) -> Optional['ASTNode']:
        """
        Search for a node and its children for a node by id.

        Arguments:
            node (ASTNode): The node to search.
            id (str): Id of node to find.

        Returns:
            ASTNode with given id if found, None otherwise.
        """
        if node.get_id() == id:
            return node

        for child in node.get_children():
            return ASTNode.find_by_id(child, id)

        return None

    @staticmethod
    def find_function_by_name(node: 'ASTNode', name: str) -> Optional['FunctionDecl']:
        """
        Search for a FunctionDecl node by function name.

        Arguments:
            node (ASTNode): The node to search.
            name (str): Name of function to find.

        Returns:
            ASTNode with given name if found, None otherwise.
        """
        if isinstance(node, FunctionDecl) and node.get_kind() == "FunctionDecl":
            if node.get_name() == name:
                return node

        for child in node.get_children():
            return ASTNode.find_function_by_name(child, name)

        return None

    @staticmethod
    def get_all_functions(node: 'ASTNode') -> List['FunctionDecl']:
        """
        Return all FunctionDecl nodes within an AST.
        """
        functions: List['FunctionDecl'] = []

        def recursive_helper(n: 'ASTNode'):
            """
            Recursive helper for finding function nodes.
            """
            if isinstance(n, FunctionDecl) and n.get_kind() == "FunctionDecl":
                functions.append(n)

            for child in n.get_children():
                recursive_helper(child)

            return

        recursive_helper(node)

        return functions

    def get_id(self) -> Optional[str]:
        return self.id

    def get_kind(self) -> str:
        return self.kind

    def set_kind(self, new_kind: str) -> None:
        self.kind = new_kind

    def get_loc(self) -> Optional[OrderedDict]:
        return self.loc

    def set_loc(self, new_loc: OrderedDict) -> None:
        """
        Source location of the AST node.

        The structure of the location dictionary is:

        ``` json
        {
            "offset": int,
            "file" str,
            "line": int,
            "col": int,
            "tokLen": int
        }
        ```
        """
        self.loc = new_loc

    def get_range(self) -> Optional[OrderedDict]:
        return self.range

    def set_range(self, new_range: OrderedDict) -> None:
        """
        Source range spanned by AST node.

        The structure of the range dictionary is:

        ``` json
        {
            "begin": {
                "offset": int,
                "col": int,
                "tokLen": int
            },
            "end": {
                "offset": int,
                "line": int,
                "col": int,
                "tokLen": int
            }
        }
        ```
        """
        self.range = new_range

    def get_children(self) -> List['ASTNode']:
        return self.children

    def get_raw(self) -> OrderedDict:
        return self.raw

    def set_raw(self, new_raw: OrderedDict) -> None:
        self.raw = new_raw

    def create_control_sequence(self) -> Optional[List[Any]]:
        pass

    def __str__(self) -> str:
        output = f"ASTNode-{self.kind}(id={self.id})"
        return output

    def __repr__(self) -> str:
        output = f"ASTNode-{self.kind}(id={self.id})"
        return output


###############################################################################
################################ Declarations #################################
###############################################################################

class Decl(ASTNode):
    """
    Base class for all declaration nodes.

    This represents one declaration (or definition), e.g.

        - variable
        - typedef
        - function
        - struct
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.name: Optional[str] = self.raw.get("name")
        self.used: Optional[bool] = self.raw.get("isUsed")

        if isinstance(self.raw["type"], OrderedDict):
            self.type: str = str(self.raw["type"]["qualType"])
        elif isinstance(self.raw["type"], str):
            self.type: str = str(self.raw["type"])
        else:
            pass

        if self.name:
            self.name = str(self.name)

        if self.used is not None:
            self.used = bool(self.used)

    def get_name(self) -> Optional[str]:
        return self.name

    def set_name(self, new_name: str) -> None:
        self.name = new_name

    def is_used(self) -> Optional[bool]:
        return self.used

    def set_used(self, is_used: bool) -> None:
        self.used = is_used

    def get_type(self) -> str:
        return self.type

    def set_type(self, new_type: str) -> None:
        self.type = new_type

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name})"
        return output


class NamedDecl(Decl):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        # TODO: build


class TypeDecl(NamedDecl):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        # TODO: build


class TypedefNameDecl(TypeDecl):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        # TODO: build


class TypedefDecl(TypedefNameDecl):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        # TODO: build


class ValueDecl(NamedDecl):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.implicit: Optional[bool] = self.raw.get("isImplicit")

        if self.implicit is not None:
            self.implicit = bool(self.implicit)

    def is_implicit(self) -> Optional[bool]:
        return self.implicit

    def set_implicit(self, implicit: bool) -> None:
        self.implicit = implicit

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, isImplicit={self.implicit}, isUsed={self.used})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, isImplicit={self.implicit}, isUsed={self.used})"
        return output


class FunctionDecl(ValueDecl):
    """
    Function declaration class.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.implicit: Optional[bool] = self.raw.get("isImplicit")
        self.definition: Optional[bool] = self.raw.get("isDefinition")

        if self.implicit is not None:
            self.implicit = bool(self.implicit)

        if self.definition is not None:
            self.definition = bool(self.definition)

        self.arguments: List[ParmVarDecl] = [node for node in self.get_children() if isinstance(node, ParmVarDecl)]
        self.body: Optional[CompoundStmt] = next((node for node in self.get_children() if isinstance(node, CompoundStmt)), None)
        self.locals: List[VarDecl] = []

        def recursive_local_finder(n: ASTNode):
            if isinstance(n, VarDecl) and n.get_kind() == "VarDecl":
                self.locals.append(n)

            for child in n.get_children():
                recursive_local_finder(child)

            return

        recursive_local_finder(self)

        self.return_type: str | None = None
        if self.type:
            if self.type.startswith("void"):
                self.return_type = None
            else:
                self.return_type = self.type.split('(')[0].strip()

        self.sys_func = False
        if self.loc:
            self.from_file: str | None = self.loc.get("includedFrom", {}).get("file")
            if self.from_file:
                self.sys_func = self.from_file.startswith("/usr") or self.from_file.startswith("/Library")

        self.control_sequence = self.create_control_sequence()

    def is_implicit(self) -> Optional[bool]:
        return self.implicit

    def set_implicit(self, implicit: bool) -> None:
        self.implicit = implicit

    def is_definition(self) -> Optional[bool]:
        return self.definition

    def set_definition(self, definition: bool) -> None:
        self.definition = definition

    def is_sys_fun(self) -> bool:
        return self.sys_func

    def get_arguments(self) -> List['ParmVarDecl']:
        return self.arguments

    def get_body(self) -> Optional['CompoundStmt']:
        return self.body

    def get_return_type(self) -> Optional[str]:
        return self.return_type

    def create_control_sequence(self) -> Optional[List[Any]]:
        # TODO: define
        return None

    def __str__(self) -> str:
        args_names = [node.get_name() for node in self.arguments]
        output = f"{self.kind}(id={self.id}, type={self.type}, return type={self.return_type} name={self.name}, isDefinition={self.definition}, isImplicit={self.implicit}, isUsed={self.used}, isSysFunc={self.sys_func}, arguments={args_names})"
        return output

    def __repr__(self) -> str:
        args_names = [node.get_name() for node in self.arguments]
        local_names = [node.get_name() for node in self.locals]
        output = f"{self.kind}(id={self.id}, type={self.type}, return type={self.return_type} name={self.name}, isDefinition={self.definition}, isImplicit={self.implicit}, isUsed={self.used}, isSysFunc={self.sys_func}, arguments={args_names}, locals={local_names})"
        return output


class VarDecl(ValueDecl):
    """
    Variable declaration class.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.implicit: Optional[bool] = self.raw.get("isImplicit")
        self.referenced: Optional[bool] = self.raw.get("isReferenced")

        if self.implicit is not None:
            self.implicit = bool(self.implicit)

        if self.referenced is not None:
            self.referenced = bool(self.referenced)

        self.control_sequence = self.create_control_sequence()

    def is_implicit(self) -> Optional[bool]:
        return self.implicit

    def set_implicit(self, implicit: bool) -> None:
        self.implicit = implicit

    def is_referenced(self) -> Optional[bool]:
        return self.referenced

    def set_referenced(self, referenced: bool) -> None:
        self.referenced = referenced

    def create_control_sequence(self) -> Optional[List[Any]]:
        # TODO: define
        return None

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, isReferenced={self.referenced}, isImplicit={self.implicit}, isUsed={self.used})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, isReferenced={self.referenced}, isImplicit={self.implicit}, isUsed={self.used})"
        return output


class ParmVarDecl(VarDecl):
    """
    Parameter variable declaration class.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        if self.children:
            raise ValueError(
                f"{self.kind} is supposed to have 0 children but has {len(self.children)} children instead.")

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, |children|={len(self.children)}, isUsed={self.used})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, |children|={len(self.children)}, isUsed={self.used})"
        return output


###############################################################################
################################## Statement ##################################
###############################################################################

class Stmt(ASTNode):
    """
    Base class for all statement nodes.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, |children|={len(self.children)})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, |children|={len(self.children)})"
        return output


class ControlFlowStmt(Stmt):
    """
    Base class for all control flow statement nodes.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class IfStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.condition: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None
        self.then_branch: Optional[ASTNode] = self.children[1] if len(
            self.children) > 1 else None
        self.else_branch: Optional[ASTNode] = self.children[2] if len(
            self.children) > 2 else None


class WhileStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.condition: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None
        self.body: Optional[ASTNode] = self.children[1] if len(
            self.children) > 1 else None


class ForStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.init: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None
        self.condition: Optional[ASTNode] = self.children[1] if len(
            self.children) > 1 else None
        self.increment: Optional[ASTNode] = self.children[2] if len(
            self.children) > 2 else None
        self.body: Optional[ASTNode] = self.children[3] if len(
            self.children) > 3 else None


class SwitchStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.condition: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None
        self.body: Optional[ASTNode] = self.children[1] if len(
            self.children) > 1 else None


class CaseStmt(SwitchStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.case_value: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None
        self.substatement: Optional[ASTNode] = self.children[1] if len(
            self.children) > 1 else None


class DefaultStmt(SwitchStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.substatement: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None


class DoStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.substatement: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None


class BreakStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class ContinueStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class ReturnStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.return_expr: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None


class GotoStmt(ControlFlowStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.target_label = raw.get("label")


class CompoundStmt(ControlFlowStmt):
    """
    Represents a compound statement block '{ ... }' containing a list of statements.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class DeclStmt(Stmt):
    """
    Adaptor class for mixing declarations with statements and expressions.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        # TODO: build


class ValueStmt(Stmt):
    """
    Represents a statement that could possibly have a value and type.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        # TODO: build


class LabelStmt(ValueStmt):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.label_name = raw.get("name")
        self.substatement: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None


###############################################################################
################################# Expressions #################################
###############################################################################

class Expr(ValueStmt):
    """
    Base class for all expression nodes.

    Expressions are subclasses of statements.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.type: Optional[str] = raw.get("type", {}).get("qualType")
        self.value_category: Optional[str] = raw.get("valueCategory")

        if self.type:
            self.type = str(self.type)

        if self.value_category:
            self.value_category = str(self.value_category)

    def get_type(self) -> Optional[str]:
        return self.type

    def set_type(self, new_type: str) -> None:
        self.type = new_type

    def get_value_category(self) -> Optional[str]:
        return self.value_category

    def set_value_category(self, new_value_category: str) -> None:
        self.value_category = new_value_category

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, valueCategory={self.value_category})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, valueCategory={self.value_category})"
        return output


class Operator(Expr):
    """
    A builtin operation expression such as "x + y" or "x <= y".
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.opcode: Optional[str] = raw.get("opcode")

        if self.opcode:
            self.opcode = str(self.opcode)

    def get_opcode(self) -> Optional[str]:
        return self.opcode

    def set_opcode(self, new_opcode: str) -> None:
        self.opcode = new_opcode

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, opcode=\"{self.opcode}\", valueCategory={self.value_category})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, opcode=\"{self.opcode}\",valueCategory={self.value_category})"
        return output


class BinaryOperator(Operator):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class UnaryOperator(Operator):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class CallExpr(Expr):
    """
    Represents a function call.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class LiteralExpr(Expr):
    """Base class for all Literal expressions."""

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.value: Any = self.raw['value']

    def get_value(self) -> Any:
        return self.value

    def set_value(self, new_value: Any) -> None:
        self.value = new_value

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, value={self.value}, valueCategory={self.value_category})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, value={self.value}, valueCategory={self.value_category})"
        return output


class IntegerLiteral(LiteralExpr):
    """Represents an integer constant."""

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.value = int(self.value)


class FloatingLiteral(LiteralExpr):
    """Represents a floating point constant."""

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)


class StringLiteral(LiteralExpr):
    """Represents a string constant."""

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.value = str(self.value)


class CharacterLiteral(LiteralExpr):
    """Represents a character constant."""

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.value = str(self.value)


class DeclRefExpr(Expr):
    """
    Represents a reference to a declared variable or function.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.name: Optional[str] = raw.get("name")

        if self.name:
            self.name = str(self.name)

        raw_ref: Optional[OrderedDict] = raw.get("referencedDecl")
        if raw_ref is None:
            raise ValueError(
                f"{self.kind} is supposed to have a referencedDecl, however one is not found. Node ID: {self.id}")

        self.referenced_decl: Optional[ASTNode] = digest(raw_ref)

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, referencedDecl id={self.referenced_decl.get_id() if self.referenced_decl else None} valueCategory={self.value_category})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, name={self.name}, referencedDecl id={self.referenced_decl.get_id() if self.referenced_decl else None} valueCategory={self.value_category})"
        return output


class CastExpr(Expr):
    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.cast_kind: Optional[str] = raw.get("castKind")

        if self.cast_kind:
            self.cast_kind = str(self.cast_kind)


class ImplicitCastExpr(CastExpr):
    """
    Represents an implicit cast inserted by the compiler.
    """

    def __init__(self, kind: str, raw: OrderedDict):
        super().__init__(kind, raw)

        self.subexpression: Optional[ASTNode] = self.children[0] if len(
            self.children) > 0 else None

    def __str__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, subexpression id={self.subexpression.get_id() if self.subexpression else None}, valueCategory={self.value_category})"
        return output

    def __repr__(self) -> str:
        output = f"{self.kind}(id={self.id}, type={self.type}, subexpression id={self.subexpression.get_id() if self.subexpression else None}, valueCategory={self.value_category})"
        return output


###############################################################################
################################# Digest Node #################################
###############################################################################


def digest(raw: OrderedDict) -> Optional[ASTNode]:
    """
    Digest a raw AST into a list of digested functions.
    """
    kind = raw.get("kind", "")

    match kind:
        case "NamedDecl":
            return NamedDecl(kind, raw)
        case "TypeDecl":
            return TypeDecl(kind, raw)
        case "TypedefNameDecl":
            return TypedefNameDecl(kind, raw)
        case "TypedefDecl":
            return TypedefDecl(kind, raw)
        case "ValueDecl":
            return ValueDecl(kind, raw)
        case "FunctionDecl":
            return FunctionDecl(kind, raw)
        case "VarDecl":
            return VarDecl(kind, raw)
        case "ParmVarDecl":
            return ParmVarDecl(kind, raw)
        case "IfStmt":
            return IfStmt(kind, raw)
        case "WhileStmt":
            return WhileStmt(kind, raw)
        case "ForStmt":
            return ForStmt(kind, raw)
        case "SwitchStmt":
            return SwitchStmt(kind, raw)
        case "CaseStmt":
            return CaseStmt(kind, raw)
        case "DefaultStmt":
            return DefaultStmt(kind, raw)
        case "DoStmt":
            return DoStmt(kind, raw)
        case "BreakStmt":
            return BreakStmt(kind, raw)
        case "ContinueStmt":
            return ContinueStmt(kind, raw)
        case "ReturnStmt":
            return ReturnStmt(kind, raw)
        case "GotoStmt":
            return GotoStmt(kind, raw)
        case "CompoundStmt":
            return CompoundStmt(kind, raw)
        case "DeclStmt":
            return DeclStmt(kind, raw)
        case "ValueStmt":
            return ValueStmt(kind, raw)
        case "LabelStmt":
            return LabelStmt(kind, raw)
        case "BinaryOperator":
            return BinaryOperator(kind, raw)
        case "UnaryOperator":
            return UnaryOperator(kind, raw)
        case "CallExpr":
            return CallExpr(kind, raw)
        case "IntegerLiteral":
            return IntegerLiteral(kind, raw)
        case "FloatingLiteral":
            return FloatingLiteral(kind, raw)
        case "StringLiteral":
            return StringLiteral(kind, raw)
        case "CharacterLiteral":
            return CharacterLiteral(kind, raw)
        case "DeclRefExpr":
            return DeclRefExpr(kind, raw)
        case "ImplicitCastExpr":
            return ImplicitCastExpr(kind, raw)
        case _:
            # TODO: unhandled item
            return ASTNode(kind, raw)

    return None
