from Command import Command

class CMD_ABORT_MOTHER_BASE(Command):

    def execute(self, data):
        return self._receiver.action(data, self.__class__.__name__)

