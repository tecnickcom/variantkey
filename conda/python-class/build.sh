echo -e "\n\n*** BUILD ***\n\n"
cd python-class && make format && ${PYTHON} setup.py install --single-version-externally-managed --record=record.txt
