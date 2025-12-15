## MooseDocs.extensions.pysyntax.PySyntax {#moosedocs.extensions.pysyntax.pysyntax}

`MooseDocs.extensions.pysyntax.PySyntax(cls)`

Helper class for extracting documentation from a python object.

### **`Info`**

Data struct for storing information about a member.

### **`_locate(cls_in: str) -> object`**

Get the module, class, or function that a string is representing.

### **`items(function=None, **kwargs)`**

Return dict() style generator to name and `Info` objects.

## **`make_extension(**kwargs)`**

Return the pysyntax extension object.